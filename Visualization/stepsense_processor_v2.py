#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
StepSense Perception Processor - V2
Role: Senior Robotics Perception Engineer

Description:
Aggregates fragmented ToF frames into a unified FootState.
Performs spatial fusion, noise filtering, and cliff detection.

Target Path: e:\OneDrive\HKUST\250909-AnklePerception\StepSense_SimtoReal\v2\stepsense_processor_v2.py
"""

import time
import threading
import queue
import numpy as np
from dataclasses import dataclass, field
from typing import Dict, Optional, List, Tuple

# Local imports - V2 version
import stepsense_hal_v2 as stepsense_hal

# --- Config & Calibration ---
CLIFF_THRESHOLD_MM = 50.0  # Height difference to trigger cliff flag
BUFFER_TIMEOUT_S = 0.15    # Drop incomplete frames after 150ms (for Serial jitter)
SMOOTHING_ALPHA = 0.2      # For median/spatial smoothing

# Re-use geometry constants from HAL for projection
FOV_DEG = stepsense_hal.FOV_DEG
TAN_HALF_FOV = stepsense_hal.TAN_HALF_FOV
NUM_ZONES = stepsense_hal.NUM_ZONES
POSES = stepsense_hal.POSES

@dataclass
class StepSenseFootState:
    is_left: bool
    frame_id: int
    timestamp_ms: float
    point_cloud: np.ndarray  # 192x3 in Foot Frame (Units: CM)
    status_mask: np.ndarray  # 192 uint8
    fused_distances: Dict[str, np.ndarray] = field(default_factory=dict) # 8x8 matrix per sensor in CM
    is_cliff: bool = False
    health_score: float = 1.0  

class StepSenseProcessor(threading.Thread):
    def __init__(self, hal_driver: stepsense_hal.StepSenseDriverBase):
        super().__init__(daemon=True)
        self.driver = hal_driver
        self.output_queue = queue.Queue(maxsize=10)
        
        # Last known complete states
        self.latest_states = {"left": None, "right": None}
        self._running = False

    def start(self):
        self._running = True
        super().start()

    def stop(self):
        self._running = False

    def get_latest_state(self, side="left") -> Optional[StepSenseFootState]:
        return self.latest_states.get(side)

    def run(self):
        print(f"[Processor] Algorithm engine started. Source: {type(self.driver).__name__}")
        
        # Sensor slots: side -> {base_name: StepSenseFrame}
        # A bundle is complete when we have "toe", "front", and "rear"
        active_bundles = {"left": {}, "right": {}}
        
        while self._running:
            # 1. Fetch fragmented frame from HAL
            frame = self.driver.get_frame(block=True, timeout=0.1)
            if not frame:
                continue
            
            # 2. Sort into Left/Right slots
            side = "left" if frame.sensor_id < 10 else "right"
            sensor_type = ""
            if "toe" in frame.name: sensor_type = "toe"
            elif "front" in frame.name: sensor_type = "front"
            elif "rear" in frame.name: sensor_type = "rear"
            
            if not sensor_type: continue 

            # logic: If we already have this sensor type in the current bundle,
            # or the bundle is too old, it means we missed some packets.
            # We should finalize if possible, then start fresh.
            current_bundle = active_bundles[side]
            now = time.time() * 1000
            
            force_flush = False
            if sensor_type in current_bundle:
                 force_flush = True
            elif current_bundle:
                 first_ts = list(current_bundle.values())[0].t_global_ms
                 if (now - first_ts) > (BUFFER_TIMEOUT_S * 1000):
                      force_flush = True
            
            if force_flush:
                 # If we have at least 2 sensors, we might still want to process,
                 # but for 3D visualization, completeness is preferred.
                 # For now, we just clear and start the new bundle.
                 active_bundles[side] = {}
                 current_bundle = active_bundles[side]

            # Addition to bundle
            current_bundle[sensor_type] = frame
            
            # 3. Check for completion (Toe + Front + Rear)
            if len(current_bundle) == 3:
                state = self._process_bundle(side, frame.frame_id, current_bundle)
                self.latest_states[side] = state
                
                # Push to output queue for downstream consumers
                if self.output_queue.full():
                    try: self.output_queue.get_nowait()
                    except: pass
                self.output_queue.put(state)
                
                # Clear bundle for next cycle
                active_bundles[side] = {}

    def _process_bundle(self, side: str, fid: int, frames: Dict[str, stepsense_hal.StepSenseFrame]) -> StepSenseFootState:
        """Fused 3 sensors into a unified FootState with feature extraction."""
        all_points = []
        all_status = []
        
        # We assume common timestamp from the last arrived frame for this ID
        t_global = list(frames.values())[0].t_global_ms
        
        # Feature Extraction Accumulators
        heights = {"front": 0.0, "rear": 0.0}
        fused_dist_cm = {}
        
        for name, frame in frames.items():
            # A. Clean and Normalize (Convert MM to CM)
            dist_cm = frame.distance_matrix / 10.0
            fused_dist_cm[frame.name] = dist_cm
            
            # B. Geometric Projection to Sensor-Local Frame (Units: CM)
            pc_local, status_flat = self._project_sensor_to_local_cm(dist_cm, frame.status_matrix)
            
            # C. Transform to Foot Frame using HAL.POSES
            pose = POSES[frame.name]
            pc_homo = np.hstack([pc_local, np.ones((64, 1))])
            pc_foot = (pose @ pc_homo.T).T[:, :3]
            
            all_points.append(pc_foot)
            all_status.append(status_flat)
            
            # D. Vertical Height Extraction
            if "front" in name:
                heights["front"] = np.mean(pc_foot[:, 1])
            elif "rear" in name:
                heights["rear"] = np.mean(pc_foot[:, 1])
                
        # E. Combine
        cloud = np.vstack(all_points)
        status = np.concatenate(all_status)
        
        # F. Feature Engineering: Cliff Detection
        is_cliff = False
        height_diff = heights["rear"] - heights["front"]
        if height_diff > (CLIFF_THRESHOLD_MM / 10.0): # Convert threshold to CM
            is_cliff = True
            
        return StepSenseFootState(
            is_left=(side == "left"),
            frame_id=fid,
            timestamp_ms=t_global,
            point_cloud=cloud,
            status_mask=status,
            fused_distances=fused_dist_cm,
            is_cliff=is_cliff,
            health_score=np.mean(status == 0)
        )

    def _project_sensor_to_local_cm(self, dm_cm: np.ndarray, sm: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Project 8x8 matrix (CM) to 3D points in sensor-local coordinate system."""
        points = []
        sm_flat = sm.flatten()
        
        for r in range(NUM_ZONES):
            for c in range(NUM_ZONES):
                dist = float(dm_cm[r, c])
                tx = (NUM_ZONES/2.0 - c - 0.5) / (NUM_ZONES/2.0) * TAN_HALF_FOV
                ty = (NUM_ZONES/2.0 - r - 0.5) / (NUM_ZONES/2.0) * TAN_HALF_FOV
                points.append([dist * tx, dist * ty, dist])
        
        return np.array(points), sm_flat


# --- Demo Entry Point ---
if __name__ == "__main__":
    # 1. Start HAL in Mock Mode
    driver = stepsense_hal.create_driver()
    driver.start()
    
    # 2. Start Processor
    proc = StepSenseProcessor(driver)
    proc.start()
    
    print("Processor standalone test running (Mock Mode)...")
    try:
        while True:
            state = proc.output_queue.get(block=True)
            side_str = "LEFT" if state.is_left else "RIGHT"
            cliff_str = "[[CLIFF ALERT!!]]" if state.is_cliff else "Safe"
            print(f"[{side_str}] ID: {state.frame_id} | Pkts: 192 | Health: {state.health_score:.2f} | Ground: {cliff_str}")
    except KeyboardInterrupt:
        proc.stop()
        driver.stop()
