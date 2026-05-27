#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
StepSense Hardware Abstraction Layer (HAL) - V2
Role: Senior Robotics System Architect

Target Path: e:\OneDrive\HKUST\250909-AnklePerception\StepSense_SimtoReal\v2\stepsense_hal_v2.py

Features:
1. Multi-environment auto-detection (Mock / Isaac Sim / Serial)
2. Global time axis synchronization (T_global = T_anchor + T_offset)
3. Bit-unpacking of 8x8 ToF distance matrices (12-bit depth, 4-bit status)
4. Non-blocking threaded I/O
5. Enhanced Isaac Sim integration for new data sources
"""

import os
import sys
import time
import struct
import threading
import platform
import queue
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Optional, Dict, Tuple

import numpy as np

import zmq

# --- Constants & Protocol Definitions ---
SYNC_WORD = 0x55AA
HEADER_LEN = 14
PAYLOAD_LEN = 136
PACKET_TOTAL_LEN = HEADER_LEN + PAYLOAD_LEN
SENSOR_ZONES = 64  # 8x8 grid
NUM_ZONES = 8      # Grid dimension

# Sensor Physical Optic Constants
FOV_DEG = 45.0
TAN_HALF_FOV = np.tan(np.deg2rad(FOV_DEG) / 2.0)

# ID Mappings (Reference from SpatialView.py)
ID_MAP = {
    1:  "toe_l",   2:  "front_l",  3:  "rear_l",
    11: "toe_r",  12: "front_r",  13: "rear_r"
}

@dataclass
class StepSenseFrame:
    sensor_id: int
    name: str
    frame_id: int       # Synchronized frame counter
    t_global_ms: float  # Master clock time
    distance_matrix: np.ndarray  # 8x8 uint16 (mm)
    status_matrix: np.ndarray    # 8x8 uint8
    raw_packet: bytes

# --- Coordinate Transformations (Sim-to-Real standard) ---
def get_pose_matrix(translation, forward, up):
    """Generates a 4x4 transformation matrix based on axis directions."""
    z_axis = np.array(forward) / np.linalg.norm(forward)
    y_axis = np.array(up) / np.linalg.norm(up)
    x_axis = np.cross(y_axis, z_axis)
    x_axis = x_axis / np.linalg.norm(x_axis)
    y_axis = np.cross(z_axis, x_axis)
    pose = np.eye(4)
    pose[0:3, 0], pose[0:3, 1], pose[0:3, 2], pose[0:3, 3] = x_axis, y_axis, z_axis, translation
    return pose

# Offsets and Poses (Shared with Visualization tools)
OFFSET_L, OFFSET_R = -15.0, 15.0
POSES = {
    "toe_l":   get_pose_matrix([10.3, 1.1, OFFSET_L], [1, 0, 0],  [0, -1, 0]),
    "front_l": get_pose_matrix([8.3, 0, OFFSET_L],    [0, -1, 0], [1, 0, 0]),
    "rear_l":  get_pose_matrix([-8.7, 0, OFFSET_L],   [0, -1, 0], [-1, 0, 0]),
    "toe_r":   get_pose_matrix([10.3, 1.1, OFFSET_R], [1, 0, 0],  [0, -1, 0]),
    "front_r": get_pose_matrix([8.3, 0, OFFSET_R],    [0, -1, 0], [1, 0, 0]),
    "rear_r":  get_pose_matrix([-8.7, 0, OFFSET_R],   [0, -1, 0], [-1, 0, 0])
}

# --- Abstract Base Class ---
class StepSenseDriverBase(ABC):
    def __init__(self):
        self.frame_queue = queue.Queue(maxsize=10)
    
    @abstractmethod
    def start(self):
        pass
    
    @abstractmethod
    def stop(self):
        pass

    @abstractmethod
    def send_command(self, cmd: str):
        """Send a command string to the hardware (ASCII or single char)."""
        pass

    def get_frame(self, block=False, timeout=None) -> Optional[StepSenseFrame]:
        """Fetch the latest parsed frame from the queue."""
        try:
            return self.frame_queue.get(block=block, timeout=timeout)
        except queue.Empty:
            return None

    def _parse_packet(self, data: bytes) -> Optional[StepSenseFrame]:
        """
        Standard protocol parser (Native S3 Layout).
        Header (14B): Magic(2), Timestamp(4), FrameID(4), SensorID(1), Len(1), Checksum(2)
        """
        # 1. Unpack Header
        magic, timestamp, frame_id, sid, plen, cs_rx = struct.unpack('<HIIBBH', data[:HEADER_LEN])
        
        # 2. Verify Checksum
        cs_calc = (sum(data[0:12]) + sum(data[HEADER_LEN:PACKET_TOTAL_LEN])) & 0xFFFF
        if cs_calc != cs_rx:
            return None
        
        # 3. Frequency & Time Axis
        # The S3 bridge already adds anchor + offset into the timestamp field
        t_global = float(timestamp)
        
        # 4. Unpack Payload (Bit-Unpacking)
        payload = data[HEADER_LEN:HEADER_LEN+128]
        raw_zones = struct.unpack('<64H', payload)
        
        distances = np.zeros(SENSOR_ZONES, dtype=np.uint16)
        statuses = np.zeros(SENSOR_ZONES, dtype=np.uint8)
        
        for i, val in enumerate(raw_zones):
            distances[i] = val & 0x0FFF
            statuses[i] = (val >> 12) & 0x0F
            
        return StepSenseFrame(
            sensor_id=sid,
            name=ID_MAP.get(sid, f"unknown_{sid}"),
            frame_id=frame_id,
            t_global_ms=t_global,
            distance_matrix=distances.reshape(8, 8),
            status_matrix=statuses.reshape(8, 8),
            raw_packet=data
        )

# --- Mock Driver (Local Development) ---
class MockDriver(StepSenseDriverBase):
    def __init__(self):
        super().__init__()
        self._running = False
        self._thread = None
        self._frame_counter = 0

    def start(self):
        self._running = True
        self._frame_counter = 0
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        if self._thread:
            # We don't join to avoid blocking the main thread if UI is hanging
            pass

    def send_command(self, cmd: str):
        print(f"[StepSense] MockDriver received command: {cmd.strip()}")

    def _run(self):
        print("[StepSense] MockDriver started (Synchronized Perception Mode)")
        while self._running:
            # Simulate a synchronized capture across all sensors
            t_ms = (time.time() * 1000) - 20 
            self._frame_counter += 1
            
            # Simulate for both feet (1-3 and 11-13)
            # Sensors 1,2,3 for same frame_id, then 11,12,13 for same frame_id
            for sid in [1, 2, 3, 11, 12, 13]:
                dist_mat = np.zeros((8, 8), dtype=np.uint16)
                
                if sid in [1, 11]: # Toe: Step in front
                    dist_mat[0:4, :] = 150 + np.random.randint(0, 5, (4, 8))
                    dist_mat[4:8, :] = 400 + np.random.randint(0, 5, (4, 8))
                elif sid in [2, 12]: # Front: Constant flat ground
                    dist_mat[:, :] = 150 + np.random.randint(0, 5, (8, 8))
                elif sid in [3, 13]: # Rear: Hanging off edge
                    dist_mat[0:4, :] = 450 + np.random.randint(0, 5, (4, 8))
                    dist_mat[4:8, :] = 150 + np.random.randint(0, 5, (4, 8))
                
                frame = StepSenseFrame(
                    sensor_id=sid,
                    name=ID_MAP[sid],
                    frame_id=self._frame_counter,
                    t_global_ms=t_ms,
                    distance_matrix=dist_mat,
                    status_matrix=np.zeros((8, 8), dtype=np.uint8),
                    raw_packet=b""
                )
                if self.frame_queue.full():
                    try: self.frame_queue.get_nowait()
                    except: pass
                self.frame_queue.put(frame)
            
            time.sleep(1.0 / 15.0) # Match hardware: 15Hz

# --- Serial Driver (Jetson / Ubuntu / Real HW) ---
class SerialDriver(StepSenseDriverBase):
    def __init__(self, port="/dev/ttyACM0", baud=2000000):
        super().__init__()
        self.port = port
        self.baud = baud
        self._running = False
        self._buffer = bytearray()
        self.ser = None
        self.log_queue = queue.Queue() # For UI visibility of serial logs

    def start(self):
        import serial
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
            self._running = True
            self._thread = threading.Thread(target=self._run, daemon=True)
            self._thread.start()
            print(f"[StepSense] SerialDriver connected to {self.port}")
        except Exception as e:
            print(f"[StepSense] Serial connection failed: {e}")

    def stop(self):
        self._running = False
        if self.ser:
            self.ser.close()

    def send_command(self, cmd: str):
        if self.ser and self.ser.is_open:
            self.ser.write(cmd.encode())
            print(f"[StepSense] Serial command sent: {cmd.strip()}")

    def _process_junk(self, limit):
        """Extracts ASCII logs from the buffer head."""
        junk = self._buffer[:limit]
        # Look for typical log markers [ or \n
        lines = junk.split(b'\n')
        for line in lines[:-1]:
            try:
                msg = line.decode('ascii', errors='ignore').strip()
                if msg: self.log_queue.put(msg)
            except: pass
        del self._buffer[:limit]

    def _run(self):
        while self._running:
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting)
                    self._buffer.extend(data)
                
                    # Slinding Window Parser: 2-byte minimum threshold
                    while len(self._buffer) >= 2:
                        idx = self._buffer.find(b'\xAA\x55') 
                        if idx != -1:
                            if idx > 0:
                                # Junk detected before sync word
                                self._process_junk(idx)
                                continue # Re-check at current idx 0
                            
                            # Sync word at 0, check if we have full packet
                            if len(self._buffer) >= PACKET_TOTAL_LEN:
                                pkt_data = bytes(self._buffer[:PACKET_TOTAL_LEN])
                                frame = self._parse_packet(pkt_data)
                                if frame:
                                    # Successful Parse: Consume full packet
                                    if self.frame_queue.full():
                                        try: self.frame_queue.get_nowait()
                                        except: pass
                                    self.frame_queue.put(frame)
                                    del self._buffer[:PACKET_TOTAL_LEN]
                                else:
                                    # [CRITICAL FIX] Checksum failed: 
                                    # Only slide 2 bytes and continue searching. 
                                    # Do NOT drop the potential real packet behind!
                                    del self._buffer[:2]
                            else:
                                # Not enough bytes for a full packet yet
                                break
                        else:
                            # No Sync Word found: Extract logs up to last \n
                            nl_idx = self._buffer.rfind(b'\n')
                            if nl_idx != -1:
                                self._process_junk(nl_idx + 1)
                            elif len(self._buffer) > 4096:
                                self._buffer.clear()
                            break
                else:
                    time.sleep(0.001)
            except Exception as e:
                if self._running:
                    print(f"[StepSense] Serial thread error: {e}")
                time.sleep(1.0)

# --- Isaac Sim Driver (Simulation) ---
class IsaacSimDriver(StepSenseDriverBase):
    def __init__(self, addr="tcp://192.168.x.x:5555"):
        """
        Driver for Isaac Sim using ZeroMQ (ZMQ).
        :param addr: The ZMQ publisher address (Ubuntu Host IP).
        """
        super().__init__()
        self.addr = addr
        self._running = False
        self._thread = None
        # Inherit frame_queue from StepSenseDriverBase

    def start(self):
        """Starts the background thread to receive ZMQ packets."""
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        print(f"[StepSense] IsaacSimDriver started. Listening on {self.addr}")

    def stop(self):
        """Stops the driver and closes ZMQ resources."""
        self._running = False
        if self._thread:
            self._thread.join(timeout=1.0)
        print("[StepSense] IsaacSimDriver stopped.")

    def send_command(self, cmd: str):
        """Simulates sending a command. No real hardware effect in sim."""
        print(f"[StepSense] Sim-Command (Dummy): {cmd.strip()}")

    def _run(self):
        """Internal receiver loop."""
        # Setup ZMQ Context and Subscriber Socket
        context = zmq.Context()
        sock = context.socket(zmq.SUB)
        
        try:
            sock.connect(self.addr)
            # Subscribe to all topics (all sensor IDs)
            sock.setsockopt_string(zmq.SUBSCRIBE, "") 
            # Non-blocking timeout to allow thread exit
            sock.setsockopt(zmq.RCVTIMEO, 500) 
        except Exception as e:
            print(f"[StepSense] ZMQ Init Error: {e}")
            return

        while self._running:
            try:
                # Receive raw binary packet from Ubuntu/Docker
                packet = sock.recv()
                
                # Check for Version 7 standard packet length (14B Header + 136B Payload = 150B)
                if len(packet) == 150:
                    # Use the common parser to decode 12-bit distance matrices
                    frame = self._parse_packet(packet)
                    
                    if frame:
                        # Push to the thread-safe queue for SpatialView/Processor
                        if self.frame_queue.full():
                            try: self.frame_queue.get_nowait()
                            except: pass
                        self.frame_queue.put(frame)
                        
            except zmq.Again:
                # No data received within timeout, just continue
                continue
            except Exception as e:
                if self._running:
                    print(f"[StepSense] ZMQ Runtime Error: {e}")
                break
        
        sock.close()
        context.term()

# --- Factory & Detection ---
def create_driver(port: Optional[str] = None) -> StepSenseDriverBase:
    """Auto-detects environment and returns appropriate driver."""
    # Check for Isaac Sim
    try:
        import omni
        if omni.app.is_running(): return IsaacSimDriver()
    except: pass
    
    # Check for Linux/Jetson (Serial)
    if platform.system() == "Linux":
        p = port if port else "/dev/ttyACM0"
        return SerialDriver(port=p)
    
    # Default to Mock
    return MockDriver()

# --- Entry Point / Demo ---
if __name__ == "__main__":
    driver = create_driver()
    driver.start()
    
    print(f"HAL logic running in: {type(driver).__name__}")
    print("Press Ctrl+C to stop.\n")
    
    try:
        while True:
            frame = driver.get_frame(block=True, timeout=1.0)
            if frame:
                print(f"[{frame.name}] T_global: {frame.t_global_ms:.2f} | Center Dist: {frame.distance_matrix[4,4]}mm")
            else:
                print("Waiting for data...")
    except KeyboardInterrupt:
        driver.stop()
        print("\nHAL stopped.")
