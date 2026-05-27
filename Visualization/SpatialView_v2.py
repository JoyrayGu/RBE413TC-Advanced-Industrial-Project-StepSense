#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
SpatialView.py (HAL Integrated Version v2)
Role: Senior Robotics Perception Architect

Target Path: e:\OneDrive\HKUST\250909-AnklePerception\StepSense_SimtoReal\v2\SpatialView_v2.py

Features:
1. V2 version with enhanced Isaac Sim integration
2. Updated to use stepsense_hal_v2 and stepsense_processor_v2
3. Path injection adapted for v2 folder structure
"""

import open3d as o3d
import numpy as np
import time
import sys
import os
import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText
import threading
import serial.tools.list_ports

# --- Path Injection & Robust Loading ---
def get_resource_path(filename):
    """Detect resource path in current dir, 3DModel subdir, script dir, or parent dir."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.abspath(os.path.join(script_dir, ".."))
    model_dir = os.path.join(script_dir, "3DModel")
    
    candidates = [
        os.path.join(script_dir, filename),
        os.path.join(model_dir, filename),
        os.path.join(parent_dir, filename),
        os.path.join(os.getcwd(), filename),
        os.path.join(os.getcwd(), "3DModel", filename)
    ]
    
    for p in candidates:
        if os.path.exists(p):
            print(f"[SpatialView_v2] Resource found: {p}")
            return p
            
    print(f"[SpatialView_v2] Warning: {filename} not found in candidates.")
    return filename

try:
    # V2: Look for modules in the same v2 directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Add current directory (v2 folder) to path
    sys.path.insert(0, script_dir)
    
    import stepsense_hal_v2 as stepsense_hal
    import stepsense_processor_v2 as stepsense_processor
    print("[SpatialView_v2] Successfully loaded v2 modules")
except ImportError as e:
    print(f"[SpatialView_v2] Critical: HAL or Processor v2 module not found: {e}")
    sys.exit(1)

# --- Configuration ---
SMOOTHING_ALPHA = 0.8
FOV_DEG = 45.0
TAN_HALF_FOV = np.tan(np.deg2rad(FOV_DEG) / 2.0)
NUM_ZONES = 8

ID_TO_NAME = stepsense_hal.ID_MAP
POSES = stepsense_hal.POSES
OFFSET_L, OFFSET_R = stepsense_hal.OFFSET_L, stepsense_hal.OFFSET_R

# --- Global Shared State ---
class SharedState:
    driver = None
    processor = None
    is_app_running = True
    view_mode = "fused" # "raw" or "fused"
    is_connected = False
    mirror_on = False
    is_dual_detected = False
    is_continuous_mode = True
    is_cliff_active = {"left": False, "right": False}
    last_fused_id = {"left": -1, "right": -1}
    state_map = {name: np.ones((8, 8)) * 15.0 for name in ID_TO_NAME.values()}
    frame_stats = {
        "total_packets": 0, 
        "last_t_global": 0, 
        "latency": 0,
        "fps_raw": 0,
        "fps_fused_l": 0,
        "fps_fused_r": 0,
        "raw_count": 0,
        "fused_count_l": 0,
        "fused_count_r": 0,
        "last_fps_time": time.time()
    }

gl = SharedState()

# --- Geometry Helpers ---

def create_disconnected_mesh(matrix_cm):
    vertices, triangles, colors = [], [], []
    vertex_index = 0
    for r in range(NUM_ZONES):
        for c in range(NUM_ZONES):
            dist = matrix_cm[r, c]
            tx_r = (NUM_ZONES/2.0 - c) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            tx_l = (NUM_ZONES/2.0 - (c + 1)) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            ty_t = (NUM_ZONES/2.0 - r) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            ty_b = (NUM_ZONES/2.0 - (r + 1)) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            v_tr, v_tl, v_bl, v_br = [dist*tx_r, dist*ty_t, dist], [dist*tx_l, dist*ty_t, dist], [dist*tx_l, dist*ty_b, dist], [dist*tx_r, dist*ty_b, dist]
            vertices.extend([v_tr, v_tl, v_bl, v_br])
            vertex_index += 4
            colors.extend([[0.1, 0.8, 0.2]] * 4)
            triangles.extend([[vertex_index-4, vertex_index-2, vertex_index-3], [vertex_index-4, vertex_index-1, vertex_index-2]])
    mesh = o3d.geometry.TriangleMesh()
    mesh.vertices, mesh.triangles, mesh.vertex_colors = o3d.utility.Vector3dVector(vertices), o3d.utility.Vector3iVector(triangles), o3d.utility.Vector3dVector(colors)
    mesh.compute_vertex_normals()
    return mesh

def create_continuous_mesh(matrix_cm):
    vertices = []
    for r in range(NUM_ZONES):
        for c in range(NUM_ZONES):
            dist = matrix_cm[r, c]
            tx = (NUM_ZONES/2.0 - c - 0.5) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            ty = (NUM_ZONES/2.0 - r - 0.5) / (NUM_ZONES/2.0) * TAN_HALF_FOV
            vertices.append([dist * tx, dist * ty, dist])
    triangles = []
    for r in range(NUM_ZONES - 1):
        for c in range(NUM_ZONES - 1):
            i_tr, i_tl, i_br, i_bl = r*8+c, r*8+c+1, (r+1)*8+c, (r+1)*8+c+1
            triangles.extend([[i_tr, i_br, i_tl], [i_tl, i_br, i_bl]])
    mesh = o3d.geometry.TriangleMesh()
    mesh.vertices, mesh.triangles = o3d.utility.Vector3dVector(vertices), o3d.utility.Vector3iVector(triangles)
    mesh.paint_uniform_color([0.1, 0.2, 0.8])
    mesh.compute_vertex_normals()
    return mesh

def create_frustum(matrix_cm):
    dist_tr, dist_tl, dist_br, dist_bl = matrix_cm[0,0], matrix_cm[0,7], matrix_cm[7,0], matrix_cm[7,7]
    v_tr, v_tl, v_br, v_bl = [dist_tr*TAN_HALF_FOV, dist_tr*TAN_HALF_FOV, dist_tr], [-dist_tl*TAN_HALF_FOV, dist_tl*TAN_HALF_FOV, dist_tl], [dist_br*TAN_HALF_FOV, -dist_br*TAN_HALF_FOV, dist_br], [-dist_bl*TAN_HALF_FOV, -dist_bl*TAN_HALF_FOV, dist_bl]
    points = [[0,0,0], v_tr, v_tl, v_br, v_bl]
    ls = o3d.geometry.LineSet()
    ls.points, ls.lines, ls.colors = o3d.utility.Vector3dVector(points), o3d.utility.Vector2iVector([[0,1],[0,2],[0,3],[0,4]]), o3d.utility.Vector3dVector([[1,0,0]]*4)
    return ls

# --- UI Panel ---

class ControlPanel(threading.Thread):
    def __init__(self):
        super().__init__(daemon=True)
        self.root = None

    def run(self):
        self.root = tk.Tk()
        self.root.title("StepSense Perception Engine (v2)")
        self.root.geometry("480x780")
        self.root.configure(bg="#2d2d2d")

        style = ttk.Style()
        style.theme_use('clam')
        style.configure("TLabel", background="#2d2d2d", foreground="#fff")
        style.configure("TRadiobutton", background="#2d2d2d", foreground="#fff")
        
        # 1. Mode Selection
        mode_frame = ttk.LabelFrame(self.root, text=" 1. Perception Engine ", padding=10)
        mode_frame.pack(fill="x", padx=10, pady=5)
        self.mode_var = tk.StringVar(value="mock")
        self.rb_mock = ttk.Radiobutton(mode_frame, text="Virtual Sim", variable=self.mode_var, value="mock", command=self.on_mode_change)
        self.rb_mock.pack(side="left", padx=10)
        self.rb_serial = ttk.Radiobutton(mode_frame, text="Real Hardware", variable=self.mode_var, value="serial", command=self.on_mode_change)
        self.rb_serial.pack(side="left", padx=10)

        # 2. Serial Configuration
        self.ser_frame = ttk.LabelFrame(self.root, text=" 2. Serial Configuration ", padding=10)
        self.ser_frame.pack(fill="x", padx=10, pady=5)
        
        port_row = ttk.Frame(self.ser_frame)
        port_row.pack(fill="x")
        self.lbl_port = ttk.Label(port_row, text="Port:")
        self.lbl_port.pack(side="left")
        self.port_combo = ttk.Combobox(port_row, width=20, state="readonly")
        self.port_combo.pack(side="left", padx=5, fill="x", expand=True)
        self.btn_refresh = ttk.Button(port_row, text="Refresh", command=self.refresh_ports)
        self.btn_refresh.pack(side="left")
        
        # Dual-Line Style Button for Port Status
        self.btn_conn = tk.Button(self.ser_frame, command=self.toggle_serial, relief="raised", bd=3)
        self.btn_conn.pack(fill="x", pady=(10,0))

        # 3. Quick Actions
        self.act_frame = ttk.LabelFrame(self.root, text=" 3. Shortcuts ", padding=10)
        self.act_frame.pack(fill="x", padx=10, pady=5)
        btn_row = ttk.Frame(self.act_frame)
        btn_row.pack(fill="x")
        
        self.btn_force = tk.Button(btn_row, text="Force (A)", command=lambda: self.send_cmd("A"), bg="#444", fg="white")
        self.btn_force.pack(side="left", expand=True, fill="x", padx=2)
        self.btn_mirror = tk.Button(btn_row, text="Mirror (off)", command=self.toggle_mirror_cmd, bg="#E57373", fg="white", font=("Arial", 9, "bold"))
        self.btn_mirror.pack(side="left", expand=True, fill="x", padx=2)
        self.btn_config = tk.Button(btn_row, text="Configuration", command=lambda: self.send_cmd("show_cfg\n"), bg="#444", fg="white")
        self.btn_config.pack(side="left", expand=True, fill="x", padx=2)

        # 4. Network Settings
        self.net_frame = ttk.LabelFrame(self.root, text=" 4. Network Settings ", padding=10)
        self.net_frame.pack(fill="x", padx=10, pady=5)
        
        ip_row = ttk.Frame(self.net_frame)
        ip_row.pack(fill="x")
        ttk.Label(ip_row, text="ZMQ Host (IP):").pack(side="left")
        self.ip_entry = ttk.Entry(ip_row)
        self.ip_entry.insert(0, "") # Default empty
        self.ip_entry.pack(side="left", fill="x", expand=True, padx=5)
        
        self.btn_net_conn = tk.Button(self.net_frame, command=self.toggle_network, relief="raised", bd=3)
        self.btn_net_conn.pack(fill="x", pady=(10,0))

        # 5. Manual Entry
        self.manual_frame = ttk.LabelFrame(self.root, text=" 5. Manual Command ", padding=10)
        self.manual_frame.pack(fill="x", padx=10, pady=5)
        self.cmd_entry = ttk.Entry(self.manual_frame)
        self.cmd_entry.pack(side="left", fill="x", expand=True, padx=5)
        self.cmd_entry.bind("<Return>", lambda e: self.on_cmd_enter())
        self.btn_send = ttk.Button(self.manual_frame, text="Send", command=self.on_cmd_enter)
        self.btn_send.pack(side="right")

        # 6. Data Source (Stream Mode)
        src_frame = ttk.LabelFrame(self.root, text=" 6. Stream Mode ", padding=10)
        src_frame.pack(fill="x", padx=10, pady=5)
        self.src_var = tk.StringVar(value="fused")
        ttk.Radiobutton(src_frame, text="Raw (Direct HAL)", variable=self.src_var, value="raw", command=self.on_source_change).pack(side="left", padx=10)
        ttk.Radiobutton(src_frame, text="Fused (Processor)", variable=self.src_var, value="fused", command=self.on_source_change).pack(side="left", padx=10)

        # 7. Stats & Console
        mon_frame = ttk.LabelFrame(self.root, text=" 7. Perception Monitor ", padding=10)
        mon_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.cliff_lbl = tk.Label(mon_frame, text=" [!] CLIFF ALERT [!] ", bg="#2d2d2d", fg="#2d2d2d", font=("Arial", 12, "bold"))
        self.cliff_lbl.pack(fill="x")
        
        self.lbl_mon = ttk.Label(mon_frame, text="T_global: -- | Latency: --", font=("Consolas", 10))
        self.lbl_mon.pack(anchor="nw")
        self.console = ScrolledText(mon_frame, bg="black", fg="#0f0", font=("Consolas", 9), height=8)
        self.console.pack(fill="both", expand=True, pady=5)

        self.refresh_ports()
        self.on_mode_change() 
        self.update_loop()
        self.root.mainloop()

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.port_combo['values'] = [f"{p.device}" for p in ports]
        if ports: self.port_combo.current(0)
        self.log(f"Synced {len(ports)} COM ports.")

    def _update_ui_states(self):
        """Orchestrate widget states and visual styles based on current mode."""
        mode = self.mode_var.get()
        is_conn = gl.is_connected

        if mode == "mock":
            # Virtual Sim Mode
            # 1. Disable Serial Section
            self.port_combo.config(state="disabled")
            self.btn_refresh.config(state="disabled")
            self.btn_conn.config(state="disabled", text="SERIAL INACTIVE\n(Virtual Mode)", bg="#666", fg="#999")
            
            # 2. Enable Network Section
            if not is_conn:
                self.ip_entry.config(state="normal")
                self.btn_net_conn.config(state="normal", bg="#FF5252", fg="white", 
                                         text="Port Closed, click to connect", font=("Arial", 11, "bold"))
            else:
                self.ip_entry.config(state="disabled")
                self.btn_net_conn.config(state="normal", bg="#50C878", fg="white", 
                                         text="Port Open, click to disconnect", font=("Arial", 11, "bold"))
            
            # 3. Handle Other Buttons
            btn_state = "normal" if is_conn else "disabled"
            self.btn_force.config(state=btn_state)
            self.btn_mirror.config(state=btn_state)
            self.btn_config.config(state=btn_state)
            self.cmd_entry.config(state=btn_state)
            self.btn_send.config(state=btn_state)

        else:
            # Real Hardware Mode
            # 1. Disable Network Section
            self.ip_entry.config(state="disabled")
            self.btn_net_conn.config(state="disabled", text="NETWORK INACTIVE\n(Hardware Mode)", bg="#666", fg="#999")

            # 2. Enable Serial Section
            if not is_conn:
                self.port_combo.config(state="readonly")
                self.btn_refresh.config(state="normal")
                self.btn_conn.config(state="normal", bg="#FF5252", fg="white", 
                                     text="Port Closed, click to connect", font=("Arial", 11, "bold"))
                self.btn_force.config(state="disabled")
                self.btn_mirror.config(state="disabled")
                self.btn_config.config(state="disabled")
                self.cmd_entry.config(state="disabled")
                self.btn_send.config(state="disabled")
            else:
                self.port_combo.config(state="disabled")
                self.btn_refresh.config(state="disabled")
                self.btn_conn.config(state="normal", bg="#50C878", fg="white", 
                                     text="Port Open, click to disconnect", font=("Arial", 11, "bold"))
                self.btn_force.config(state="normal")
                self.btn_mirror.config(state="normal")
                self.btn_config.config(state="normal")
                self.cmd_entry.config(state="normal")
                self.btn_send.config(state="normal")

        # Update Mirror Button Specifics
        if gl.mirror_on:
            self.btn_mirror.config(text="Mirror (on)", bg="#50C878") # Emerald Green
        else:
            self.btn_mirror.config(text="Mirror (off)", bg="#E57373") # Muted Red

    def on_mode_change(self):
        mode = self.mode_var.get()
        self.log(f"Switching to {mode} mode...")
        
        if gl.processor: gl.processor.stop()
        if gl.driver: gl.driver.stop()
        
        gl.driver = None
        gl.processor = None
        gl.is_connected = False
        gl.mirror_on = False
        
        self._update_ui_states()
    
    def toggle_network(self):
        """Handle connection for Virtual Sim (Mock or Isaac Sim)."""
        if not gl.is_connected:
            ip = self.ip_entry.get().strip()
            try:
                if not ip:
                    self.log("Starting MockDriver (15Hz local data)...")
                    gl.driver = stepsense_hal.MockDriver()
                else:
                    addr = f"tcp://{ip}:5555"
                    self.log(f"Connecting to Isaac Sim @ {addr}...")
                    gl.driver = stepsense_hal.IsaacSimDriver(addr=addr)
                
                gl.driver.start()
                
                if gl.view_mode == "fused":
                    gl.processor = stepsense_processor.StepSenseProcessor(gl.driver)
                    gl.processor.start()
                    self.log("Virtual Link Established (Fused Mode).")
                else:
                    self.log("Virtual Link Established (Raw Mode).")
                
                gl.is_connected = True
            except Exception as e:
                self.log(f"Network Error: {e}")
        else:
            if gl.processor: gl.processor.stop()
            if gl.driver: gl.driver.stop()
            gl.driver = None
            gl.processor = None
            gl.is_connected = False
            self.log("Virtual Link Terminated.")
        
        self._update_ui_states()
    
    def on_source_change(self):
        gl.view_mode = self.src_var.get()
        self.log(f"View mode set to: {gl.view_mode}")
        
        # --- Handle Processor Lifecycle on Switch ---
        if gl.view_mode == "raw":
            if gl.processor:
                self.log("Stopping Processor to release Driver Queue...")
                gl.processor.stop()
                gl.processor = None
        else:
            if gl.driver and not gl.processor:
                self.log("Starting Processor for Fused Stream...")
                gl.processor = stepsense_processor.StepSenseProcessor(gl.driver)
                gl.processor.start()
    
    def toggle_serial(self):
        if not gl.is_connected:
            port = self.port_combo.get()
            if not port:
                 self.log("Error: No port selected.")
                 return
            self.log(f"Opening {port}...")
            try:
                gl.driver = stepsense_hal.SerialDriver(port=port)
                gl.driver.start()
                
                if gl.view_mode == "fused":
                    gl.processor = stepsense_processor.StepSenseProcessor(gl.driver)
                    gl.processor.start()
                    self.log("Connected Successfully (Fused Mode).")
                else:
                    self.log("Connected Successfully (Raw Mode).")
                
                gl.is_connected = True
            except Exception as e:
                self.log(f"Failed: {e}")
        else:
            if gl.processor: gl.processor.stop()
            if gl.driver: gl.driver.stop()
            gl.driver = None
            gl.processor = None
            gl.is_connected = False
            self.log("Serial Interface Stopped.")
        
        self._update_ui_states()

    def toggle_mirror_cmd(self):
        """Toggle mirror status and UI state."""
        gl.mirror_on = not gl.mirror_on
        cmd = "set_mirror ON\n" if gl.mirror_on else "set_mirror OFF\n"
        self.send_cmd(cmd)
        self._update_ui_states()

    def on_cmd_enter(self):
        cmd = self.cmd_entry.get()
        if cmd:
            self.send_cmd(cmd)
            self.cmd_entry.delete(0, tk.END)

    def send_cmd(self, cmd):
        d = gl.driver
        if d and gl.is_connected:
            d.send_command(cmd)
            self.log(f"CMD > {cmd.strip()}")
        else:
            self.log("Error: Hardware not connected.")

    def log(self, msg):
        """Thread-safe logging with widget survival check."""
        try:
            if self.root.winfo_exists():
                now = time.strftime("%H:%M:%S")
                self.console.insert(tk.END, f"[{now}] {msg}\n")
                self.console.see(tk.END)
        except Exception:
            pass # Suppress errors during window destruction

    def update_loop(self):
        """Monitor UI updates (Statistics) with safety checks."""
        if not gl.is_app_running: return
        try:
            if self.root and self.root.winfo_exists():
                # Calculate FPS every 1 second
                now = time.time()
                dt = now - gl.frame_stats["last_fps_time"]
                if dt >= 1.0:
                    gl.frame_stats["fps_raw"] = gl.frame_stats["raw_count"] / dt
                    gl.frame_stats["fps_fused_l"] = gl.frame_stats["fused_count_l"] / dt
                    gl.frame_stats["fps_fused_r"] = gl.frame_stats["fused_count_r"] / dt
                    gl.frame_stats["raw_count"] = 0
                    gl.frame_stats["fused_count_l"] = 0
                    gl.frame_stats["fused_count_r"] = 0
                    gl.frame_stats["last_fps_time"] = now

                f_fps = f"L {gl.frame_stats['fps_fused_l']:.1f} | R {gl.frame_stats['fps_fused_r']:.1f}"
                fps_str = f"FPS(Raw): {gl.frame_stats['fps_raw']:.1f} | FPS(Fused): {f_fps}"
                self.lbl_mon.config(text=f"{fps_str}\nT_global: {gl.frame_stats['last_t_global']:.2f} ms | Latency: {gl.frame_stats['latency']:.1f} ms")
                
                # Cliff Alert Visual Feedback
                has_cliff = gl.is_cliff_active["left"] or gl.is_cliff_active["right"]
                if has_cliff:
                    self.cliff_lbl.config(bg="red", fg="white")
                else:
                    self.cliff_lbl.config(bg="#2d2d2d", fg="#2d2d2d")
                    
                if isinstance(gl.driver, stepsense_hal.SerialDriver):
                     while not gl.driver.log_queue.empty():
                          try: 
                              msg = gl.driver.log_queue.get_nowait()
                              self.log(f"HW: {msg}")
                          except: break
                self.root.after(100, self.update_loop)
        except Exception:
            pass # Widget likely destroyed

# --- Main Rendering ---

def main():
    cp = ControlPanel()
    cp.start()

    MODEL_FILENAME = "Simplified_Single_Ankle.obj"
    MODEL_PATH = get_resource_path(MODEL_FILENAME)
    print(f"[SpatialView_v2] Loading mesh: {MODEL_PATH}")
    
    try: 
        foot_l = o3d.io.read_triangle_mesh(MODEL_PATH)
        if not foot_l.has_vertices(): raise ValueError("Empty mesh file")
        foot_l.compute_vertex_normals()
        foot_l.paint_uniform_color([0.2, 0.6, 0.9])
    except Exception as e:
        print(f"[SpatialView_v2] ERROR: {e}")
        foot_l = o3d.geometry.TriangleMesh.create_coordinate_frame(size=12)
    
    foot_l.translate([0, 0, OFFSET_L])
    foot_r = o3d.geometry.TriangleMesh(foot_l)
    foot_r.paint_uniform_color([0.9, 0.4, 0.2])
    foot_r.translate([0, 0, OFFSET_R - OFFSET_L])

    vis = o3d.visualization.VisualizerWithKeyCallback()
    vis.create_window(window_name="StepSense SpatialView (v2)")
    vis.get_render_option().background_color = np.array([0.9, 0.9, 0.9]) # Lighter background
    vis.add_geometry(foot_l)

    geos = {}
    for name in ID_TO_NAME.values():
        m = create_disconnected_mesh(np.ones((8,8))*15)
        f = create_frustum(np.ones((8,8))*15)
        m.transform(POSES[name]); f.transform(POSES[name])
        geos[name] = (m, f)
        if "_l" in name: vis.add_geometry(m); vis.add_geometry(f)

    def toggle_mesh(v):
        gl.is_continuous_mode = not gl.is_continuous_mode
        return False
    vis.register_key_callback(ord("T"), toggle_mesh)

    try:
        while vis.poll_events():
            # [CRITICAL] Local Snapshot Pattern to prevent 'NoneType' race conditions
            proc = gl.processor
            drv = gl.driver
            v_mode = gl.view_mode

            if v_mode == "fused" and proc:
                # Fused Branch: Aggregate states from local snapshot 'proc'
                for side in ["left", "right"]:
                    state = proc.get_latest_state(side)
                    if not state: continue
                    
                    # Update stats ONLY if this is a NEW frame
                    if state.frame_id != gl.last_fused_id[side]:
                        gl.last_fused_id[side] = state.frame_id
                        if side == "left":
                            gl.frame_stats["fused_count_l"] += 1
                        else:
                            gl.frame_stats["fused_count_r"] += 1
                        
                        gl.frame_stats["last_t_global"] = state.timestamp_ms
                        gl.frame_stats["latency"] = (time.time() * 1000) - state.timestamp_ms
                        gl.is_cliff_active[side] = state.is_cliff

                    # Bulk update all 3 meshes for this foot using fused_distances
                    names = ["toe_l", "front_l", "rear_l"] if side=="left" else ["toe_r", "front_r", "rear_r"]
                    
                    if not gl.is_dual_detected and side == "right":
                        gl.is_dual_detected = True
                        vis.add_geometry(foot_r)
                        for n in names:
                            vis.add_geometry(geos[n][0]); vis.add_geometry(geos[n][1])

                    for name in names:
                        if name not in state.fused_distances: continue
                        dm_cm = state.fused_distances[name]
                        m_old, f_old = geos[name]
                        
                        m_new = create_continuous_mesh(dm_cm) if gl.is_continuous_mode else create_disconnected_mesh(dm_cm)
                        f_new = create_frustum(dm_cm)
                        m_new.transform(POSES[name]); f_new.transform(POSES[name])
                        
                        m_old.vertices, m_old.triangles, m_old.vertex_colors = m_new.vertices, m_new.triangles, m_new.vertex_colors
                        f_old.points = f_new.points
                        vis.update_geometry(m_old); vis.update_geometry(f_old)

            elif v_mode == "raw" and drv:
                # Raw Branch: Using local snapshot 'drv'
                frame = drv.get_frame(block=False)
                while frame:
                    sid, name = frame.sensor_id, frame.name
                    gl.frame_stats["last_t_global"] = frame.t_global_ms
                    gl.frame_stats["latency"] = (time.time() * 1000) - frame.t_global_ms
                    gl.is_cliff_active["left" if sid < 10 else "right"] = False 
                    gl.frame_stats["raw_count"] += 1

                    if not gl.is_dual_detected and sid > 10:
                        gl.is_dual_detected = True
                        vis.add_geometry(foot_r)
                        for n in ["toe_r", "front_r", "rear_r"]:
                            vis.add_geometry(geos[n][0]); vis.add_geometry(geos[n][1])

                    if name in gl.state_map:
                        cm = frame.distance_matrix / 10.0
                        gl.state_map[name] = gl.state_map[name] * (1-SMOOTHING_ALPHA) + cm * SMOOTHING_ALPHA
                        m_old, f_old = geos[name]
                        
                        m_new = create_continuous_mesh(gl.state_map[name]) if gl.is_continuous_mode else create_disconnected_mesh(gl.state_map[name])
                        f_new = create_frustum(gl.state_map[name])
                        m_new.transform(POSES[name]); f_new.transform(POSES[name])
                        
                        m_old.vertices, m_old.triangles, m_old.vertex_colors = m_new.vertices, m_new.triangles, m_new.vertex_colors
                        f_old.points = f_new.points
                        vis.update_geometry(m_old); vis.update_geometry(f_old)
                    frame = drv.get_frame(block=False)

            vis.update_renderer()
            time.sleep(0.005)
    except Exception as e:
        print(f"[SpatialView_v2] Main loop exit signal: {e}")
    finally:
        gl.is_app_running = False
        print("[SpatialView_v2] Shutting down modules...")
        if gl.processor: 
             try: gl.processor.stop()
             except: pass
        if gl.driver: 
             try: gl.driver.stop()
             except: pass
        vis.destroy_window()
        print("[SpatialView_v2] All systems stopped.")

if __name__ == "__main__":
    main()
