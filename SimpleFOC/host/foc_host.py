#!/usr/bin/env python3
"""SimpleFOC desktop monitor and remote controller."""

from __future__ import annotations

import argparse
import csv
import math
import queue
import threading
import time
from collections import deque
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Callable, Optional

import serial
from serial.tools import list_ports

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import ImageChops, ImageGrab

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure


MAX_SPEED_RPM = 90.0
PLOT_SPEED_LIMIT = MAX_SPEED_RPM * 1.1


@dataclass
class Telemetry:
    host_time: float
    mcu_ms: int
    enabled: int
    mode: int
    target_rpm: float
    actual_rpm: float
    target_deg: float
    position_deg: float
    uq: float


def parse_telemetry(raw: str) -> Optional[Telemetry]:
    text = raw.strip()
    if not text.startswith("FOC,"):
        return None

    fields = text.split(",")
    if len(fields) != 9:
        return None

    try:
        return Telemetry(
            host_time=time.monotonic(),
            mcu_ms=int(fields[1]),
            enabled=int(fields[2]),
            mode=int(fields[3]),
            target_rpm=float(fields[4]),
            actual_rpm=float(fields[5]),
            target_deg=float(fields[6]),
            position_deg=float(fields[7]),
            uq=float(fields[8]),
        )
    except ValueError:
        return None


class SerialClient:
    def __init__(
        self,
        on_message: Callable[[str], None],
        on_error: Callable[[str], None],
    ) -> None:
        self.on_message = on_message
        self.on_error = on_error
        self._serial: Optional[serial.Serial] = None
        self._reader: Optional[threading.Thread] = None
        self._heartbeat: Optional[threading.Thread] = None
        self._stop = threading.Event()
        self._write_lock = threading.Lock()

    def connect(self, port: str, baudrate: int = 115200) -> None:
        self.close(send_stop=False)
        self._serial = serial.Serial(port, baudrate, timeout=0.1, write_timeout=0.5)
        self._stop.clear()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()
        self._heartbeat = threading.Thread(target=self._heartbeat_loop, daemon=True)
        self._heartbeat.start()

    def _read_loop(self) -> None:
        while not self._stop.is_set():
            try:
                if self._serial is None:
                    break
                raw = self._serial.readline()
                if raw:
                    self.on_message(raw.decode("utf-8", errors="replace").strip())
            except serial.SerialException as exc:
                if not self._stop.is_set():
                    self.on_error(f"串口读取失败: {exc}")
                break

    def send(self, command: str) -> None:
        with self._write_lock:
            if self._serial is None or not self._serial.is_open:
                raise serial.SerialException("串口未连接")
            self._serial.write((command.strip() + "\n").encode("ascii"))
            self._serial.flush()

    def _heartbeat_loop(self) -> None:
        while not self._stop.wait(0.5):
            try:
                self.send("K")
            except serial.SerialException as exc:
                if not self._stop.is_set():
                    self.on_error(f"心跳发送失败: {exc}")
                return

    def close(self, send_stop: bool = True) -> None:
        if send_stop and self._serial is not None and self._serial.is_open:
            try:
                self._serial.write(b"STOP\n")
                self._serial.flush()
            except serial.SerialException:
                pass

        self._stop.set()
        if self._serial is not None:
            try:
                self._serial.close()
            except serial.SerialException:
                pass
            self._serial = None

        if self._reader is not None and self._reader.is_alive():
            self._reader.join(timeout=0.5)
        self._reader = None
        if self._heartbeat is not None and self._heartbeat.is_alive():
            self._heartbeat.join(timeout=0.5)
        self._heartbeat = None


class DemoClient:
    """Protocol-compatible simulator used by --demo."""

    def __init__(
        self,
        on_message: Callable[[str], None],
        on_error: Callable[[str], None],
    ) -> None:
        self.on_message = on_message
        self.on_error = on_error
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()
        self._enabled = 0
        self._mode = 0
        self._target_rpm = 0.0
        self._target_deg = 0.0
        self._actual_rpm = 0.0
        self._position_deg = 0.0
        self._started = time.monotonic()

    def connect(self, port: str = "DEMO", baudrate: int = 115200) -> None:
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def send(self, command: str) -> None:
        text = command.strip().upper()
        with self._lock:
            if text == "STOP":
                self._enabled = 0
                self._mode = 0
                self._target_rpm = 0.0
            elif text in {"START", "EN"}:
                self._enabled = 1
                self._mode = 1
                self._target_rpm = 0.0
            elif text.startswith("V,"):
                self._enabled = 1
                self._mode = 3
                self._target_rpm = max(
                    -MAX_SPEED_RPM, min(MAX_SPEED_RPM, float(text[2:]))
                )
            elif text.startswith("F,"):
                self._enabled = 1
                self._mode = 1
                self._target_rpm = max(
                    -MAX_SPEED_RPM, min(MAX_SPEED_RPM, float(text[2:]))
                )
            elif text.startswith("S,"):
                self._enabled = 1
                self._mode = 4
                self._target_rpm = max(
                    -MAX_SPEED_RPM, min(MAX_SPEED_RPM, float(text[2:]))
                )
            elif text.startswith("P,"):
                self._enabled = 1
                self._mode = 2
                self._target_deg = max(-720.0, min(720.0, float(text[2:])))

    def _run(self) -> None:
        last = time.monotonic()
        while not self._stop.wait(0.1):
            now = time.monotonic()
            dt = now - last
            last = now
            with self._lock:
                if self._mode in (1, 3, 4) and self._enabled:
                    speed_target = self._target_rpm
                elif self._mode == 2 and self._enabled:
                    error = self._target_deg - self._position_deg
                    speed_target = max(
                        -MAX_SPEED_RPM, min(MAX_SPEED_RPM, error * 0.2)
                    )
                else:
                    speed_target = 0.0

                response = 5.0 if self._enabled else 2.0
                self._actual_rpm += (speed_target - self._actual_rpm) * min(
                    1.0, dt * response
                )
                self._position_deg += self._actual_rpm * 6.0 * dt
                uq = 0.02 * (speed_target - self._actual_rpm)
                elapsed = int((now - self._started) * 1000)
                line = (
                    f"FOC,{elapsed},{self._enabled},{self._mode},"
                    f"{speed_target:.4f},{self._actual_rpm:.4f},"
                    f"{self._target_deg:.2f},{self._position_deg:.2f},{uq:.4f}"
                )
            self.on_message(line)

    def close(self, send_stop: bool = True) -> None:
        if send_stop:
            self.send("STOP")
        self._stop.set()
        if self._thread is not None and self._thread.is_alive():
            self._thread.join(timeout=0.5)
        self._thread = None


class FocHostApp:
    BG = "#eef2f6"
    PANEL = "#ffffff"
    TEXT = "#18212b"
    MUTED = "#667085"
    ACCENT = "#176b87"
    TARGET = "#d97706"
    ACTUAL = "#0f766e"
    DANGER = "#b42318"

    def __init__(self, root: tk.Tk, demo: bool = False) -> None:
        self.root = root
        self.demo = demo
        self.client: Optional[object] = None
        self.connected = False
        self.controller_ready = False
        self.motion_buttons: list[ttk.Button] = []
        self.events: queue.Queue[tuple[str, str]] = queue.Queue()
        self.samples: deque[Telemetry] = deque(maxlen=1000)
        self.start_ms: Optional[int] = None
        self.plot_start_time: Optional[float] = None
        self.last_sample: Optional[Telemetry] = None
        self.log_widget: Optional[tk.Text] = None
        self.chart_dirty = False

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="115200")
        self.speed_var = tk.DoubleVar(value=10.0)
        self.position_var = tk.DoubleVar(value=90.0)
        self.connection_text = tk.StringVar(value="未连接")
        self.target_rpm_text = tk.StringVar(value="--")
        self.actual_rpm_text = tk.StringVar(value="--")
        self.position_text = tk.StringVar(value="--")
        self.position_total_text = tk.StringVar(value="--")
        self.uq_text = tk.StringVar(value="--")
        self.mode_text = tk.StringVar(value="STOPPED")
        self.error_text = tk.StringVar(value="--")
        self.quality_text = tk.StringVar(value="--")
        self.safety_text = tk.StringVar(value="等待连接")

        self.root.title("SimpleFOC 速度与位置遥控台")
        self.root.geometry("1180x720")
        self.root.minsize(980, 620)
        self.root.configure(bg=self.BG)
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

        self._configure_style()
        self._build_ui()
        self._refresh_ports()
        self.root.after(50, self._drain_events)
        self.root.after(250, self._refresh_chart)

        if demo:
            self.root.after(350, self.connect)

    def _configure_style(self) -> None:
        style = ttk.Style(self.root)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        style.configure("TFrame", background=self.BG)
        style.configure("Panel.TFrame", background=self.PANEL)
        style.configure("TLabel", background=self.BG, foreground=self.TEXT)
        style.configure(
            "Panel.TLabel", background=self.PANEL, foreground=self.TEXT
        )
        style.configure(
            "Title.TLabel",
            background=self.BG,
            foreground=self.TEXT,
            font=("Segoe UI", 18, "bold"),
        )
        style.configure(
            "Metric.TLabel",
            background=self.PANEL,
            foreground=self.ACCENT,
            font=("Segoe UI", 20, "bold"),
        )
        style.configure(
            "Caption.TLabel",
            background=self.PANEL,
            foreground=self.MUTED,
            font=("Segoe UI", 9),
        )
        style.configure(
            "Accent.TButton",
            font=("Segoe UI", 10, "bold"),
            foreground="white",
            background=self.ACCENT,
        )
        style.map("Accent.TButton", background=[("active", "#12556c")])
        style.configure(
            "Danger.TButton",
            font=("Segoe UI", 10, "bold"),
            foreground="white",
            background=self.DANGER,
        )
        style.map("Danger.TButton", background=[("active", "#8f1e14")])

    def _build_ui(self) -> None:
        outer = ttk.Frame(self.root, padding=14)
        outer.pack(fill="both", expand=True)

        header = ttk.Frame(outer)
        header.pack(fill="x", pady=(0, 10))
        ttk.Label(header, text="SimpleFOC Control Studio", style="Title.TLabel").pack(
            side="left"
        )
        chip = tk.Label(
            header,
            textvariable=self.connection_text,
            bg="#dfe8ef",
            fg=self.TEXT,
            padx=12,
            pady=5,
            font=("Segoe UI", 9, "bold"),
        )
        chip.pack(side="right")

        connection_bar = ttk.Frame(outer, style="Panel.TFrame", padding=(12, 8))
        connection_bar.pack(fill="x", pady=(0, 10))
        self._build_connection(connection_bar)

        self.workspace = ttk.Notebook(outer)
        self.workspace.pack(fill="both", expand=True)

        dashboard = ttk.Frame(self.workspace, padding=10)
        position_page = ttk.Frame(self.workspace, padding=10)
        log_page = ttk.Frame(self.workspace, padding=10)
        self.workspace.add(dashboard, text="运行总览")
        self.workspace.add(position_page, text="位置控制")
        self.workspace.add(log_page, text="通信日志")
        self.workspace.bind(
            "<<NotebookTabChanged>>",
            lambda event: setattr(self, "chart_dirty", True),
        )

        body = ttk.Frame(dashboard)
        body.pack(fill="both", expand=True)
        body.columnconfigure(0, weight=0)
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)

        side = ttk.Frame(body, style="Panel.TFrame", padding=16)
        side.grid(row=0, column=0, sticky="nsw", padx=(0, 14))
        side.configure(width=330)
        side.pack_propagate(False)
        side.grid_propagate(False)
        self.side_panel = side

        chart_panel = ttk.Frame(body, style="Panel.TFrame", padding=12)
        chart_panel.grid(row=0, column=1, sticky="nsew")
        chart_panel.pack_propagate(False)
        chart_panel.grid_propagate(False)
        self.chart_panel = chart_panel

        self._build_controls(side)
        self._build_metrics(side)
        self._build_chart(chart_panel)
        self._build_position_page(position_page)
        self._build_log_page(log_page)
        self._set_motion_controls(False)

    def _build_connection(self, parent: ttk.Frame) -> None:
        ttk.Label(
            parent, text="设备连接", style="Panel.TLabel",
            font=("Segoe UI", 10, "bold")
        ).pack(side="left", padx=(0, 10))

        ttk.Label(parent, text="端口", style="Caption.TLabel").pack(
            side="left", padx=(0, 4)
        )
        self.port_combo = ttk.Combobox(
            parent, textvariable=self.port_var, width=10, state="readonly"
        )
        self.port_combo.pack(side="left")
        ttk.Button(parent, text="刷新", width=6, command=self._refresh_ports).pack(
            side="left", padx=4
        )

        ttk.Label(parent, text="波特率", style="Caption.TLabel").pack(
            side="left", padx=(8, 4)
        )
        ttk.Entry(parent, textvariable=self.baud_var, width=9).pack(side="left")
        self.connect_button = ttk.Button(
            parent, text="连接", style="Accent.TButton", command=self.connect
        )
        self.connect_button.pack(side="left", padx=(10, 4))
        self.disconnect_button = ttk.Button(
            parent, text="断开", command=self.disconnect, state="disabled"
        )
        self.disconnect_button.pack(side="left")
        ttk.Label(
            parent,
            text="后台心跳 · 断线 3 秒自动停机",
            style="Caption.TLabel",
        ).pack(side="right")

    def _build_controls(self, parent: ttk.Frame) -> None:
        ttk.Label(
            parent, text="遥控控制", style="Panel.TLabel",
            font=("Segoe UI", 11, "bold")
        ).pack(anchor="w")

        ttk.Label(parent, text="速度 (RPM)", style="Caption.TLabel").pack(
            anchor="w", pady=(10, 2)
        )
        speed_row = ttk.Frame(parent, style="Panel.TFrame")
        speed_row.pack(fill="x")
        ttk.Spinbox(
            speed_row,
            from_=-MAX_SPEED_RPM,
            to=MAX_SPEED_RPM,
            increment=1,
            textvariable=self.speed_var,
            width=10,
        ).pack(side="left", fill="x", expand=True)
        self.speed_apply_button = ttk.Button(
            speed_row, text="应用", width=8, command=self.apply_speed
        )
        self.speed_apply_button.pack(side="left", padx=(6, 0))
        self.motion_buttons.append(self.speed_apply_button)

        ttk.Label(parent, text="位置 (°，相对对齐零位)", style="Caption.TLabel").pack(
            anchor="w", pady=(10, 2)
        )
        pos_row = ttk.Frame(parent, style="Panel.TFrame")
        pos_row.pack(fill="x")
        ttk.Spinbox(
            pos_row,
            from_=-720,
            to=720,
            increment=5,
            textvariable=self.position_var,
            width=10,
        ).pack(side="left", fill="x", expand=True)
        self.position_apply_button = ttk.Button(
            pos_row, text="应用", width=8, command=self.apply_position
        )
        self.position_apply_button.pack(side="left", padx=(6, 0))
        self.motion_buttons.append(self.position_apply_button)

        action_row = ttk.Frame(parent, style="Panel.TFrame")
        action_row.pack(fill="x", pady=(12, 6))
        self.start_button = ttk.Button(
            action_row, text="启动/零速", style="Accent.TButton", command=self.start
        )
        self.start_button.pack(side="left", fill="x", expand=True)
        self.motion_buttons.append(self.start_button)
        ttk.Button(
            action_row, text="停止", style="Danger.TButton", command=self.stop
        ).pack(side="left", padx=(6, 0))

        ttk.Button(parent, text="保存 CSV", command=self.save_csv).pack(
            fill="x", pady=(4, 0)
        )
        self.zero_button = ttk.Button(
            parent, text="当前位置设为零", command=self.zero_position
        )
        self.zero_button.pack(fill="x", pady=(6, 0))
        self.motion_buttons.append(self.zero_button)
        ttk.Label(
            parent,
            textvariable=self.safety_text,
            style="Caption.TLabel",
            wraplength=270,
        ).pack(anchor="w", pady=(8, 0))

    def _build_metrics(self, parent: ttk.Frame) -> None:
        ttk.Separator(parent).pack(fill="x", pady=(14, 8))
        ttk.Label(
            parent, text="实时状态", style="Panel.TLabel",
            font=("Segoe UI", 11, "bold")
        ).pack(anchor="w")

        grid = ttk.Frame(parent, style="Panel.TFrame")
        grid.pack(fill="x", pady=(8, 0))
        grid.columnconfigure(0, weight=1)
        grid.columnconfigure(1, weight=1)

        self._metric(grid, "目标速度", self.target_rpm_text, 0, 0)
        self._metric(grid, "实际速度", self.actual_rpm_text, 0, 1)
        self._metric(grid, "当前位置", self.position_text, 1, 0)
        self._metric(grid, "Uq 电压", self.uq_text, 1, 1)

        mode_row = ttk.Frame(parent, style="Panel.TFrame")
        mode_row.pack(fill="x", pady=(8, 0))
        ttk.Label(mode_row, text="模式", style="Caption.TLabel").pack(side="left")
        ttk.Label(
            mode_row,
            textvariable=self.mode_text,
            style="Panel.TLabel",
            font=("Segoe UI", 10, "bold"),
        ).pack(side="right")
        ttk.Label(mode_row, text="速度误差", style="Caption.TLabel").pack(
            side="left", padx=(16, 4)
        )
        ttk.Label(
            mode_row,
            textvariable=self.error_text,
            style="Panel.TLabel",
            font=("Segoe UI", 10, "bold"),
        ).pack(side="left")

        total_row = ttk.Frame(parent, style="Panel.TFrame")
        total_row.pack(fill="x", pady=(6, 0))
        ttk.Label(total_row, text="多圈位置", style="Caption.TLabel").pack(
            side="left"
        )
        ttk.Label(
            total_row,
            textvariable=self.position_total_text,
            style="Panel.TLabel",
            font=("Segoe UI", 9, "bold"),
        ).pack(side="right")

        quality_row = ttk.Frame(parent, style="Panel.TFrame")
        quality_row.pack(fill="x", pady=(6, 0))
        ttk.Label(quality_row, text="速度质量", style="Caption.TLabel").pack(
            side="left"
        )
        ttk.Label(
            quality_row,
            textvariable=self.quality_text,
            style="Panel.TLabel",
            font=("Segoe UI", 9, "bold"),
        ).pack(side="right")

    def _metric(
        self,
        parent: ttk.Frame,
        title: str,
        value_var: tk.StringVar,
        row: int,
        column: int,
    ) -> None:
        card = tk.Frame(parent, bg="#f7f9fb", padx=10, pady=8)
        card.grid(
            row=row,
            column=column,
            sticky="nsew",
            padx=(0 if column == 0 else 5, 0),
            pady=(0, 8),
        )
        ttk.Label(card, text=title, style="Caption.TLabel").pack(anchor="w")
        ttk.Label(card, textvariable=value_var, style="Metric.TLabel").pack(
            anchor="w"
        )

    def _build_chart(self, parent: ttk.Frame) -> None:
        title_row = ttk.Frame(parent, style="Panel.TFrame")
        title_row.pack(fill="x", padx=4, pady=(0, 4))
        ttk.Label(
            title_row,
            text="速度响应",
            style="Panel.TLabel",
            font=("Segoe UI", 12, "bold"),
        ).pack(side="left")
        ttk.Button(title_row, text="清空曲线", command=self.clear_chart).pack(
            side="right"
        )

        self.figure = Figure(figsize=(8, 5), dpi=100, facecolor=self.PANEL)
        self.axis = self.figure.add_subplot(111)
        self.axis.set_facecolor(self.PANEL)
        self.axis.set_xlabel("Time (s)")
        self.axis.set_ylabel("Speed (RPM)")
        self.axis.grid(True, alpha=0.25)
        self.axis.set_ylim(-PLOT_SPEED_LIMIT, PLOT_SPEED_LIMIT)
        self.axis.set_xlim(0, 30)
        (self.target_line,) = self.axis.plot(
            [], [], color=self.TARGET, linewidth=1.8, label="Target"
        )
        (self.actual_line,) = self.axis.plot(
            [], [], color=self.ACTUAL, linewidth=2.2, label="Actual"
        )
        self.axis.legend(loc="upper right", frameon=False)
        self.figure.tight_layout()

        self.canvas = FigureCanvasTkAgg(self.figure, master=parent)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=4, pady=4)

    def _build_position_page(self, parent: ttk.Frame) -> None:
        parent.columnconfigure(0, weight=0)
        parent.columnconfigure(1, weight=1)
        parent.rowconfigure(0, weight=1)

        controls = ttk.Frame(parent, style="Panel.TFrame", padding=18)
        controls.grid(row=0, column=0, sticky="nsw", padx=(0, 12))
        controls.configure(width=310)
        controls.pack_propagate(False)
        controls.grid_propagate(False)

        ttk.Label(
            controls,
            text="位置遥控",
            style="Panel.TLabel",
            font=("Segoe UI", 14, "bold"),
        ).pack(anchor="w")
        ttk.Label(
            controls,
            text="目标位置相对当前机械零位，可用“设为零”重新建立坐标。",
            style="Caption.TLabel",
            wraplength=270,
        ).pack(anchor="w", pady=(4, 16))

        ttk.Label(controls, text="当前位置", style="Caption.TLabel").pack(anchor="w")
        ttk.Label(
            controls, textvariable=self.position_text, style="Metric.TLabel"
        ).pack(anchor="w")
        ttk.Label(
            controls,
            textvariable=self.position_total_text,
            style="Caption.TLabel",
        ).pack(anchor="w", pady=(0, 12))

        ttk.Label(controls, text="目标角度 (°)", style="Caption.TLabel").pack(
            anchor="w"
        )
        ttk.Spinbox(
            controls,
            from_=-720,
            to=720,
            increment=5,
            textvariable=self.position_var,
        ).pack(fill="x", pady=(3, 8))
        self.position_page_apply_button = ttk.Button(
            controls,
            text="应用位置",
            style="Accent.TButton",
            command=self.apply_position,
        )
        self.position_page_apply_button.pack(fill="x", pady=4)
        self.motion_buttons.append(self.position_page_apply_button)
        self.position_page_zero_button = ttk.Button(
            controls, text="当前位置设为零", command=self.zero_position
        )
        self.position_page_zero_button.pack(fill="x", pady=4)
        self.motion_buttons.append(self.position_page_zero_button)
        ttk.Button(
            controls, text="停止输出", style="Danger.TButton", command=self.stop
        ).pack(fill="x", pady=(10, 4))

        chart_panel = ttk.Frame(parent, style="Panel.TFrame", padding=12)
        chart_panel.grid(row=0, column=1, sticky="nsew")
        chart_panel.pack_propagate(False)
        ttk.Label(
            chart_panel,
            text="位置跟踪",
            style="Panel.TLabel",
            font=("Segoe UI", 12, "bold"),
        ).pack(anchor="w", padx=4)

        self.position_figure = Figure(
            figsize=(7, 4), dpi=100, facecolor=self.PANEL
        )
        self.position_axis = self.position_figure.add_subplot(111)
        self.position_axis.set_facecolor(self.PANEL)
        self.position_axis.set_xlabel("Time (s)")
        self.position_axis.set_ylabel("Position (deg)")
        self.position_axis.grid(True, alpha=0.25)
        self.position_axis.set_xlim(0, 30)
        self.position_axis.set_ylim(-800, 800)
        (self.position_target_line,) = self.position_axis.plot(
            [], [], color=self.TARGET, linewidth=1.8, label="Target"
        )
        (self.position_actual_line,) = self.position_axis.plot(
            [], [], color=self.ACTUAL, linewidth=2.2, label="Actual"
        )
        self.position_axis.legend(loc="upper right", frameon=False)
        self.position_figure.tight_layout()
        self.position_canvas = FigureCanvasTkAgg(
            self.position_figure, master=chart_panel
        )
        self.position_canvas.get_tk_widget().pack(
            fill="both", expand=True, padx=4, pady=4
        )

    def _build_log_page(self, parent: ttk.Frame) -> None:
        panel = ttk.Frame(parent, style="Panel.TFrame", padding=12)
        panel.pack(fill="both", expand=True)

        toolbar = ttk.Frame(panel, style="Panel.TFrame")
        toolbar.pack(fill="x", pady=(0, 8))
        ttk.Label(
            toolbar,
            text="通信与保护事件",
            style="Panel.TLabel",
            font=("Segoe UI", 12, "bold"),
        ).pack(side="left")
        ttk.Button(toolbar, text="清空日志", command=self.clear_log).pack(
            side="right"
        )

        text_frame = tk.Frame(panel, bg="#111820")
        text_frame.pack(fill="both", expand=True)
        scrollbar = ttk.Scrollbar(text_frame)
        scrollbar.pack(side="right", fill="y")
        self.log_widget = tk.Text(
            text_frame,
            wrap="word",
            bg="#111820",
            fg="#d8e1e8",
            insertbackground="#d8e1e8",
            selectbackground="#28566d",
            relief="flat",
            padx=10,
            pady=8,
            font=("Consolas", 9),
            yscrollcommand=scrollbar.set,
        )
        self.log_widget.pack(side="left", fill="both", expand=True)
        scrollbar.configure(command=self.log_widget.yview)

        ttk.Label(
            panel,
            text="协议：V,<rpm> 平滑速度  |  P,<deg> 位置  |  "
                 "STOP 停机  |  ZERO 位置归零  |  K 心跳",
            style="Caption.TLabel",
        ).pack(anchor="w", pady=(8, 0))
        self._append_log("上位机已启动，等待串口连接")

    def _refresh_ports(self) -> None:
        ports = list(list_ports.comports())
        values = [port.device for port in ports]
        self.port_combo["values"] = values

        if self.port_var.get() in values or not values:
            return

        preferred = next(
            (
                port.device
                for port in ports
                if "CH340" in (port.description or "").upper()
                or "USB-SERIAL" in (port.description or "").upper()
            ),
            values[0],
        )
        self.port_var.set(preferred)

    def _append_log(self, message: str) -> None:
        if self.log_widget is None:
            return
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_widget.insert("end", f"[{timestamp}] {message}\n")
        self.log_widget.see("end")

    def clear_log(self) -> None:
        if self.log_widget is not None:
            self.log_widget.delete("1.0", "end")

    def _set_motion_controls(self, enabled: bool) -> None:
        state = "normal" if enabled else "disabled"
        for button in self.motion_buttons:
            button.configure(state=state)

    def connect(self) -> None:
        if self.connected:
            return

        try:
            baud = int(self.baud_var.get())
            self.client = (
                DemoClient(self._queue_line, self._queue_error)
                if self.demo
                else SerialClient(self._queue_line, self._queue_error)
            )
            self.client.connect(self.port_var.get() or "DEMO", baud)
            self.clear_chart()
            self.connected = True
            self.controller_ready = False
            self.connection_text.set(
                "DEMO · 等待状态"
                if self.demo
                else f"{self.port_var.get()} · 等待 READY"
            )
            self.safety_text.set("后台心跳已启动，通信中断 3 秒后电机自动停机")
            self.connect_button.configure(state="disabled")
            self.disconnect_button.configure(state="normal")
            self._set_motion_controls(False)
            self.client.send("STOP")
            self._append_log(
                f"已连接 {self.port_var.get() or 'DEMO'}，已发送 STOP"
            )
        except Exception as exc:
            self.client = None
            self.connected = False
            messagebox.showerror("连接失败", str(exc))

    def disconnect(self) -> None:
        if self.client is not None:
            try:
                self.client.close(send_stop=True)
            except Exception:
                pass
        self.client = None
        self.connected = False
        self.controller_ready = False
        self.connection_text.set("未连接")
        self.safety_text.set("已断开，电机已发送 STOP")
        self.connect_button.configure(state="normal")
        self.disconnect_button.configure(state="disabled")
        self._set_motion_controls(False)
        self._append_log("串口已断开，电机已发送 STOP")

    def _send(self, command: str) -> None:
        if not self.connected or self.client is None:
            messagebox.showwarning("未连接", "请先连接串口")
            return
        if not self.controller_ready and command.strip().upper() != "STOP":
            messagebox.showwarning("控制器未就绪", "等待对齐完成和 READY 状态")
            return
        try:
            self.client.send(command)
            self.safety_text.set(f"已发送: {command}")
            self._append_log(f"TX: {command}")
        except Exception as exc:
            messagebox.showerror("发送失败", str(exc))
            self.disconnect()

    def start(self) -> None:
        self._send("START")

    def stop(self) -> None:
        self._send("STOP")

    def apply_speed(self) -> None:
        try:
            rpm = max(
                -MAX_SPEED_RPM, min(MAX_SPEED_RPM, float(self.speed_var.get()))
            )
        except (ValueError, tk.TclError):
            messagebox.showerror(
                "参数错误", f"速度必须是 -{MAX_SPEED_RPM:.0f} 到 "
                f"{MAX_SPEED_RPM:.0f} RPM"
            )
            return
        self.speed_var.set(rpm)
        self._send(f"V,{rpm:.4f}")

    def apply_position(self) -> None:
        try:
            degrees = max(-720.0, min(720.0, float(self.position_var.get())))
        except (ValueError, tk.TclError):
            messagebox.showerror("参数错误", "位置必须是 -720 到 720 度")
            return
        self.position_var.set(degrees)
        self._send(f"P,{degrees:.3f}")

    def zero_position(self) -> None:
        self._send("ZERO")
        self.position_var.set(0.0)

    def _queue_line(self, line: str) -> None:
        self.events.put(("line", line))

    def _queue_error(self, message: str) -> None:
        self.events.put(("error", message))

    def _drain_events(self) -> None:
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break

            if kind == "error":
                self.safety_text.set(value)
                self.disconnect()
                continue

            sample = parse_telemetry(value)
            if sample is None:
                if value:
                    self._append_log(f"RX: {value}")
                    if value.startswith("Aligning") or "SimpleFOC STM32" in value:
                        self.clear_chart()
                        self.controller_ready = False
                        self.connection_text.set(
                            f"{self.port_var.get()} · 对齐中"
                        )
                        self.safety_text.set("控制器正在对齐，运动命令暂时禁用")
                        self._set_motion_controls(False)
                    elif value.startswith("READY"):
                        self.controller_ready = True
                        self.connection_text.set(
                            f"{self.port_var.get()} · READY"
                        )
                        self.safety_text.set("控制器已完成对齐，可以开始遥控")
                        self._set_motion_controls(True)
                    if value.startswith("EVT,"):
                        self.safety_text.set(value.replace(",", " · ", 1))
                    elif value.startswith("STALL"):
                        self.safety_text.set(value)
                continue

            self.samples.append(sample)
            self.last_sample = sample
            if not self.controller_ready:
                self.controller_ready = True
                self.connection_text.set(
                    f"{self.port_var.get()} · READY"
                    if not self.demo else "DEMO · READY"
                )
                self.safety_text.set("控制器遥测正常，可以开始遥控")
                self._set_motion_controls(True)
            self._update_metrics(sample)
            self.chart_dirty = True

        self.root.after(50, self._drain_events)

    def _refresh_chart(self) -> None:
        if self.chart_dirty and self.samples:
            self._redraw()
            self.chart_dirty = False
        self.root.after(250, self._refresh_chart)

    def _update_metrics(self, sample: Telemetry) -> None:
        self.target_rpm_text.set(f"{sample.target_rpm:+.1f}")
        self.actual_rpm_text.set(f"{sample.actual_rpm:+.1f}")
        wrapped = (sample.position_deg + 180.0) % 360.0 - 180.0
        self.position_text.set(f"{wrapped:+.1f}°")
        self.position_total_text.set(f"{sample.position_deg:+.1f}°")
        self.uq_text.set(f"{sample.uq:+.2f} V")
        modes = {
            0: "STOPPED",
            1: "FOC SPEED",
            2: "POSITION",
            3: "V/F SPEED",
            4: "TORQUE SPEED",
        }
        mode = modes.get(sample.mode, f"MODE {sample.mode}")
        self.mode_text.set(mode if sample.enabled else f"{mode} / DISABLED")
        self.error_text.set(f"{sample.target_rpm - sample.actual_rpm:+.1f} RPM")

        recent_10s = list(self.samples)[-100:]
        if len(recent_10s) >= 10:
            rms = math.sqrt(
                sum(
                    (item.target_rpm - item.actual_rpm) ** 2
                    for item in recent_10s
                )
                / len(recent_10s)
            )
            recent_5s = recent_10s[-50:]
            ripple = max(item.actual_rpm for item in recent_5s) - min(
                item.actual_rpm for item in recent_5s
            )
            self.quality_text.set(f"RMS {rms:.2f} · Δ {ripple:.2f} RPM")

    def _redraw(self) -> None:
        if not self.samples:
            return

        if self.plot_start_time is None:
            self.plot_start_time = self.samples[0].host_time

        xs = [sample.host_time - self.plot_start_time for sample in self.samples]
        targets = [sample.target_rpm for sample in self.samples]
        actuals = [sample.actual_rpm for sample in self.samples]
        self.target_line.set_data(xs, targets)
        self.actual_line.set_data(xs, actuals)
        positions = [sample.position_deg for sample in self.samples]
        target_positions = [sample.target_deg for sample in self.samples]
        self.position_target_line.set_data(xs, target_positions)
        self.position_actual_line.set_data(xs, positions)
        current = xs[-1]
        window_end = 30.0 if current < 30.0 else math.ceil(current / 30.0) * 30.0
        self.axis.set_xlim(window_end - 30.0, window_end)
        self.axis.set_ylim(-PLOT_SPEED_LIMIT, PLOT_SPEED_LIMIT)
        self.position_axis.set_xlim(window_end - 30.0, window_end)
        active_page = self.workspace.index("current")
        if active_page == 0:
            self.canvas.draw_idle()
        elif active_page == 1:
            self.position_canvas.draw_idle()

    def clear_chart(self) -> None:
        self.samples.clear()
        self.start_ms = None
        self.plot_start_time = None
        self.target_line.set_data([], [])
        self.actual_line.set_data([], [])
        self.position_target_line.set_data([], [])
        self.position_actual_line.set_data([], [])
        self.axis.set_xlim(0, 30)
        self.axis.set_ylim(-PLOT_SPEED_LIMIT, PLOT_SPEED_LIMIT)
        self.position_axis.set_xlim(0, 30)
        self.canvas.draw_idle()
        self.position_canvas.draw_idle()

    def save_csv(self) -> None:
        if not self.samples:
            messagebox.showinfo("没有数据", "当前没有可保存的遥测数据")
            return

        default_name = f"foc_log_{datetime.now():%Y%m%d_%H%M%S}.csv"
        filename = filedialog.asksaveasfilename(
            title="保存遥测数据",
            defaultextension=".csv",
            initialfile=default_name,
            filetypes=[("CSV", "*.csv")],
        )
        if not filename:
            return

        with Path(filename).open("w", newline="", encoding="utf-8-sig") as handle:
            writer = csv.writer(handle)
            writer.writerow(
                [
                    "host_time",
                    "mcu_ms",
                    "enabled",
                    "mode",
                    "target_rpm",
                    "actual_rpm",
                    "target_deg",
                    "position_deg",
                    "uq",
                ]
            )
            for sample in self.samples:
                writer.writerow(
                    [
                        f"{sample.host_time:.6f}",
                        sample.mcu_ms,
                        sample.enabled,
                        sample.mode,
                        f"{sample.target_rpm:.6f}",
                        f"{sample.actual_rpm:.6f}",
                        f"{sample.target_deg:.4f}",
                        f"{sample.position_deg:.4f}",
                        f"{sample.uq:.6f}",
                    ]
                )

    def on_close(self) -> None:
        self.disconnect()
        self.root.destroy()


def self_test() -> None:
    sample = parse_telemetry(
        "FOC,12345,1,1,30.0000,29.5000,0.00,12.34,0.42"
    )
    assert sample is not None
    assert sample.mcu_ms == 12345
    assert sample.enabled == 1
    assert math.isclose(sample.actual_rpm, 29.5)
    assert parse_telemetry("READY: V,<rpm>") is None

    demo_lines: list[str] = []
    demo = DemoClient(demo_lines.append, lambda message: None)
    demo.connect()
    demo.send("V,12.5")
    time.sleep(0.35)
    demo.close()
    demo_samples = [parse_telemetry(line) for line in demo_lines]
    demo_samples = [sample for sample in demo_samples if sample is not None]
    assert any(sample.enabled and math.isclose(sample.target_rpm, 12.5)
               for sample in demo_samples)

    print("self-test: OK")


def serial_probe(port: str, baudrate: int = 115200) -> None:
    deadline = time.monotonic() + 4.0
    received = 0
    with serial.Serial(port, baudrate, timeout=0.2, write_timeout=0.5) as link:
        link.write(b"STOP\n")
        link.flush()
        while time.monotonic() < deadline:
            line = link.readline().decode("utf-8", errors="replace").strip()
            if line:
                print(line)
                if line.startswith("FOC,"):
                    received += 1
                    if received >= 4:
                        break

    if received == 0:
        raise RuntimeError("4 秒内未收到 FOC 遥测")
    print(f"serial-probe: OK ({received} frames)")


def serial_speed_test(
    port: str,
    baudrate: int = 115200,
    targets: tuple[float, ...] = (10.0, 30.0),
    mode: str = "openloop",
) -> None:
    def summarize(samples: list[Telemetry], target: float) -> tuple[float, float, float]:
        speeds = [sample.actual_rpm for sample in samples]
        mean_speed = sum(speeds) / len(speeds)
        rms_error = math.sqrt(
            sum((target - speed) ** 2 for speed in speeds) / len(speeds)
        )
        peak_to_peak = max(speeds) - min(speeds)
        return mean_speed, rms_error, peak_to_peak

    with serial.Serial(port, baudrate, timeout=0.15, write_timeout=0.5) as link:
        link.write(b"STOP\n")
        link.flush()
        time.sleep(0.4)

        try:
            for target in targets:
                command = {"openloop": "V", "torque": "S", "foc": "F"}[mode]
                link.write(f"{command},{target:.3f}\n".encode("ascii"))
                link.flush()

                samples: list[Telemetry] = []
                started = time.monotonic()
                next_heartbeat = started
                settle_seconds = max(
                    2.5,
                    abs(target) * 2.0 * math.pi / 60.0 / 1.5 + 0.5,
                )
                collect_seconds = 5.0

                while time.monotonic() - started < settle_seconds + collect_seconds:
                    now = time.monotonic()
                    if now >= next_heartbeat:
                        link.write(b"K\n")
                        link.flush()
                        next_heartbeat = now + 0.5

                    line = link.readline().decode("utf-8", errors="replace").strip()
                    sample = parse_telemetry(line)
                    if sample is not None and now - started >= settle_seconds:
                        samples.append(sample)

                if len(samples) < 20:
                    raise RuntimeError(
                        f"{target:.1f} RPM 测试只收到 {len(samples)} 帧有效数据"
                    )

                mean_speed, rms_error, ripple = summarize(samples, target)
                print(
                    f"{target:.1f} RPM: mean={mean_speed:.2f}, "
                    f"RMS={rms_error:.2f}, peak-to-peak={ripple:.2f} RPM"
                )

                error_limit = max(2.0, abs(target) * 0.15)
                ripple_limit = max(4.0, abs(target) * 0.20)
                if abs(mean_speed - target) > error_limit or ripple > ripple_limit:
                    speed_text = ", ".join(
                        f"{sample.actual_rpm:.2f}" for sample in samples
                    )
                    uq_text = ", ".join(f"{sample.uq:.2f}" for sample in samples)
                    position_speeds: list[float] = []
                    for previous, current in zip(samples, samples[1:]):
                        delta_ms = (current.mcu_ms - previous.mcu_ms) & 0xFFFFFFFF
                        if delta_ms > 0:
                            rpm = (
                                (current.position_deg - previous.position_deg)
                                / 360.0
                                * 60_000.0
                                / delta_ms
                            )
                            position_speeds.append(rpm)
                    position_text = ", ".join(
                        f"{speed:.2f}" for speed in position_speeds
                    )
                    print(f"actual samples: [{speed_text}]")
                    print(f"uq samples: [{uq_text}]")
                    print(f"position-derived speeds: [{position_text}]")
                    raise RuntimeError(
                        f"{target:.1f} RPM 未达到平滑标准，测试提前终止"
                    )
        finally:
            link.write(b"STOP\n")
            link.flush()

    print("speed-test: PASS")


def serial_heartbeat_test(port: str, baudrate: int = 115200) -> None:
    messages: queue.Queue[str] = queue.Queue()
    errors: queue.Queue[str] = queue.Queue()
    client = SerialClient(messages.put, errors.put)
    samples: list[Telemetry] = []

    try:
        client.connect(port, baudrate)
        client.send("STOP")
        time.sleep(0.3)
        client.send("V,10")

        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline:
            try:
                line = messages.get(timeout=0.2)
            except queue.Empty:
                continue
            if line.startswith("EVT,HEARTBEAT_TIMEOUT"):
                raise RuntimeError("后台心跳未能阻止 MCU 超时停机")
            if line.startswith("EVT,") or line.startswith("STALL"):
                print(f"event: {line}")
            sample = parse_telemetry(line)
            if sample is not None:
                samples.append(sample)
                if len(samples) <= 12:
                    print(
                        f"sample: enabled={sample.enabled}, mode={sample.mode}, "
                        f"target={sample.target_rpm:.2f}, "
                        f"actual={sample.actual_rpm:.2f}, uq={sample.uq:.2f}"
                    )
    finally:
        client.close(send_stop=True)

    if not errors.empty():
        raise RuntimeError(errors.get())
    if len(samples) < 30:
        raise RuntimeError(f"5 秒内只收到 {len(samples)} 帧")
    if not samples[-1].enabled:
        raise RuntimeError("测试结束时电机已意外失能")
    print(f"heartbeat-test: PASS ({len(samples)} frames, motor stayed enabled)")


def serial_speed_sweep(
    port: str,
    baudrate: int = 115200,
    minimum: int = 20,
    maximum: int = 90,
    step: int = 10,
) -> None:
    last_stable = 0.0
    targets = tuple(range(minimum, maximum + 1, max(1, step)))

    with serial.Serial(port, baudrate, timeout=0.15, write_timeout=0.5) as link:
        ready_deadline = time.monotonic() + 5.0
        ready_seen = False
        while time.monotonic() < ready_deadline:
            line = link.readline().decode("utf-8", errors="replace").strip()
            if line.startswith("READY") or line.startswith("FOC,"):
                ready_seen = True
                break
        if not ready_seen:
            time.sleep(4.0)
        link.reset_input_buffer()

        link.write(b"STOP\n")
        link.flush()
        time.sleep(0.2)
        link.reset_input_buffer()

        for _ in range(4):
            link.write(b"K\n")
            link.flush()
            time.sleep(0.12)
            link.readline()
        link.reset_input_buffer()

        try:
            link.write(b"V,10\n")
            link.flush()
            time.sleep(0.8)
            previous_target = 10.0

            for target in targets:
                link.write(f"V,{target}\n".encode("ascii"))
                link.flush()
                samples: list[Telemetry] = []
                started = time.monotonic()
                next_heartbeat = started
                speed_delta = abs(target - previous_target) * 2.0 * math.pi / 60.0
                settle_seconds = max(0.8, speed_delta / 1.5 + 0.4)

                while time.monotonic() - started < settle_seconds + 1.0:
                    now = time.monotonic()
                    if now >= next_heartbeat:
                        link.write(b"K\n")
                        link.flush()
                        next_heartbeat = now + 0.5

                    line = link.readline().decode("utf-8", errors="replace").strip()
                    if line.startswith("EVT,") or line.startswith("STALL"):
                        print(f"{target:3d} RPM: event: {line}")
                    sample = parse_telemetry(line)
                    if sample is not None and now - started >= settle_seconds:
                        samples.append(sample)

                if not samples:
                    break

                speeds = [sample.actual_rpm for sample in samples]
                mean_speed = sum(speeds) / len(speeds)
                enabled = samples[-1].enabled == 1
                print(f"{target:3d} RPM: mean={mean_speed:6.2f}")
                if not enabled or mean_speed < target * 0.85:
                    break
                last_stable = float(target)
                previous_target = float(target)
        finally:
            link.write(b"STOP\n")
            link.flush()

    print(f"speed-sweep: last stable target = {last_stable:.0f} RPM")


def gui_smoke_test() -> None:
    root = tk.Tk()
    root.withdraw()
    app = FocHostApp(root, demo=True)
    deadline = time.monotonic() + 1.0
    while time.monotonic() < deadline:
        root.update()
        time.sleep(0.02)
    assert app.last_sample is not None
    assert app.log_widget is not None
    app.on_close()
    print("gui-smoke-test: OK")


def layout_test(duration_seconds: float = 5.0) -> None:
    root = tk.Tk()
    app = FocHostApp(root, demo=True)
    root.update()
    time.sleep(1.2)
    root.update()

    widgets = {
        "window": root,
        "sidebar": app.side_panel,
        "chart": app.chart_panel,
        "speed_button": app.speed_apply_button,
        "start_button": app.start_button,
        "speed_canvas": app.canvas.get_tk_widget(),
    }

    def geometry_snapshot() -> dict[str, tuple[int, int, int, int]]:
        return {
            name: (
                widget.winfo_rootx(),
                widget.winfo_rooty(),
                widget.winfo_width(),
                widget.winfo_height(),
            )
            for name, widget in widgets.items()
        }

    def screenshot() -> object:
        root.update_idletasks()
        bbox = (
            root.winfo_rootx(),
            root.winfo_rooty(),
            root.winfo_rootx() + root.winfo_width(),
            root.winfo_rooty() + root.winfo_height(),
        )
        return ImageGrab.grab(bbox=bbox)

    snapshots: list[dict[str, tuple[int, int, int, int]]] = []
    before = screenshot()
    frame_count = max(10, int(duration_seconds / 0.5))
    for frame in range(frame_count):
        root.update()
        if frame == frame_count // 2:
            app.workspace.select(1)
            root.update()
            time.sleep(0.3)
            app.workspace.select(0)
            root.update()
        time.sleep(0.5)
        snapshots.append(geometry_snapshot())
    after = screenshot()

    output_dir = Path(__file__).resolve().parent
    before.save(output_dir / "layout_test_before.png")
    after.save(output_dir / "layout_test_after.png")
    changed_pixels = ImageChops.difference(before, after).getbbox()
    app.on_close()

    first = snapshots[0]
    unstable = {
        name: values
        for name in first
        if len({snapshot[name] for snapshot in snapshots}) > 1
        for values in [[snapshot[name] for snapshot in snapshots]]
    }
    print(f"layout-test frames={len(snapshots)} changed_pixels={changed_pixels}")
    if unstable:
        print(f"layout-test unstable widgets={unstable}")
        raise RuntimeError("窗口控件在图表刷新期间发生位移或尺寸变化")
    print("layout-test: PASS")


def main() -> None:
    parser = argparse.ArgumentParser(description="SimpleFOC desktop monitor")
    parser.add_argument("--demo", action="store_true", help="run without hardware")
    parser.add_argument("--self-test", action="store_true", help="run parser test")
    parser.add_argument("--serial-test", metavar="PORT", help="probe serial telemetry")
    parser.add_argument(
        "--heartbeat-test", metavar="PORT", help="verify background heartbeat"
    )
    parser.add_argument(
        "--speed-sweep", metavar="PORT", help="find maximum stable speed"
    )
    parser.add_argument("--sweep-min", type=int, default=20)
    parser.add_argument("--sweep-max", type=int, default=90)
    parser.add_argument("--sweep-step", type=int, default=10)
    parser.add_argument(
        "--gui-smoke-test", action="store_true", help="build the GUI without hardware"
    )
    parser.add_argument(
        "--layout-test", action="store_true", help="verify geometry during live redraw"
    )
    parser.add_argument("--layout-seconds", type=float, default=5.0)
    parser.add_argument("--speed-test", metavar="PORT", help="run 10/30 RPM test")
    parser.add_argument(
        "--speed-targets",
        default="10,30",
        help="comma-separated targets for --speed-test",
    )
    parser.add_argument(
        "--speed-mode",
        choices=("foc", "openloop", "torque"),
        default="openloop",
        help="controller used by --speed-test",
    )
    args = parser.parse_args()

    if args.self_test:
        self_test()
        return

    if args.serial_test:
        serial_probe(args.serial_test)
        return

    if args.heartbeat_test:
        serial_heartbeat_test(args.heartbeat_test)
        return

    if args.speed_sweep:
        serial_speed_sweep(
            args.speed_sweep,
            minimum=args.sweep_min,
            maximum=args.sweep_max,
            step=args.sweep_step,
        )
        return

    if args.gui_smoke_test:
        gui_smoke_test()
        return

    if args.layout_test:
        layout_test(args.layout_seconds)
        return

    if args.speed_test:
        targets = tuple(float(value) for value in args.speed_targets.split(","))
        serial_speed_test(
            args.speed_test,
            targets=targets,
            mode=args.speed_mode,
        )
        return

    root = tk.Tk()
    FocHostApp(root, demo=args.demo)
    root.mainloop()


if __name__ == "__main__":
    main()
