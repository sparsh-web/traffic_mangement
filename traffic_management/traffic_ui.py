#!/usr/bin/env python3
"""
traffic_ui.py
FINAL FIXED VERSION — Stable, no glitches
Horizontal bar chart comparing:
    - Normal
    - Final Adaptive
    - Average Adaptive
    - Max Adaptive
One clean section per road.
"""

import sys
import os
import subprocess
import numpy as np
import pandas as pd
from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg

CSV_PATH = "traffic_data.csv"
EXECUTABLE = "traffic.exe"
REFRESH_MS = 600   # slower refresh to avoid glitching


class TimingUI(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Traffic Timing Comparison — Normal vs Adaptive")
        self.resize(1300, 900)

        self.process = None
        self.roads = []
        self.data = {}
        self.plot_widgets = {}

        pg.setConfigOptions(antialias=True)

        self.build_ui()

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_from_csv)
        self.timer.start(REFRESH_MS)

    def build_ui(self):
        root = QtWidgets.QVBoxLayout()

        # ====================== TOP BAR ======================
        bar = QtWidgets.QHBoxLayout()
        btn_start = QtWidgets.QPushButton("Start")
        btn_start.clicked.connect(self.start_sim)
        bar.addWidget(btn_start)

        btn_stop = QtWidgets.QPushButton("Stop")
        btn_stop.clicked.connect(self.stop_sim)
        bar.addWidget(btn_stop)

        btn_restart = QtWidgets.QPushButton("Restart")
        btn_restart.clicked.connect(self.restart_sim)
        bar.addWidget(btn_restart)

        bar.addStretch()
        self.status = QtWidgets.QLabel("Status: Idle")
        bar.addWidget(self.status)
        root.addLayout(bar)

        # ====================== SCROLL AREA ======================
        self.container = QtWidgets.QWidget()
        self.road_layout = QtWidgets.QVBoxLayout()
        self.container.setLayout(self.road_layout)

        scroll = QtWidgets.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(self.container)
        root.addWidget(scroll)

        wrapper = QtWidgets.QWidget()
        wrapper.setLayout(root)
        self.setCentralWidget(wrapper)

    # ====================== CONTROL FUNCTIONS ======================
    def reset_ui(self):
        for i in reversed(range(self.road_layout.count())):
            w = self.road_layout.itemAt(i).widget()
            if w:
                w.setParent(None)

        self.roads = []
        self.data = {}
        self.plot_widgets = {}

    def start_sim(self):
        if self.process:
            return

        self.reset_ui()

        # fresh CSV
        with open(CSV_PATH, "w") as f:
            f.write("cycle,road,arrived,passed,waiting,emergency,adaptive_green\n")

        try:
            self.process = subprocess.Popen([EXECUTABLE])
            self.status.setText("Status: Running")
        except:
            self.status.setText("ERROR: Could not start traffic.exe")

    def stop_sim(self):
        if self.process:
            self.process.kill()
            self.process = None
            self.status.setText("Status: Stopped")

    def restart_sim(self):
        self.stop_sim()
        self.start_sim()

    # ====================== CSV HANDLER ======================
    def update_from_csv(self):
        if not os.path.exists(CSV_PATH):
            return

        try:
            df = pd.read_csv(CSV_PATH)
        except:
            return

        if df.empty:
            return

        # identify fixed normal timing
        fixed_green = df["adaptive_green"].value_counts().idxmax()

        roads = sorted(df["road"].unique())

        # If UI not built for all roads → build it
        if set(roads) != set(self.roads):
            self.build_road_sections(roads)
            return   # IMPORTANT — wait for next cycle

        # Fill the data
        for r in roads:
            sub = df[df["road"] == r]
            adaptive_vals = sub[sub["adaptive_green"] != fixed_green]["adaptive_green"]

            if adaptive_vals.empty:
                final_val = 0
                avg_val = 0
                max_val = 0
            else:
                final_val = int(adaptive_vals.iloc[-1])
                avg_val = float(adaptive_vals.mean())
                max_val = int(adaptive_vals.max())

            self.data[r] = {
                "normal": fixed_green,
                "final": final_val,
                "avg": avg_val,
                "max": max_val
            }

        self.redraw()

        if self.process and self.process.poll() is not None:
            self.process = None
            self.status.setText("Status: Finished")

    # ====================== BUILD ROAD UI ======================
    def build_road_sections(self, roads):
        self.reset_ui()
        self.roads = roads

        for r in roads:
            group = QtWidgets.QGroupBox(f"Road {r} — Timing Comparison")
            vbox = QtWidgets.QVBoxLayout()

            plot = pg.PlotWidget()
            plot.setLabel("left", "Timings")
            plot.setLabel("bottom", "Seconds")
            vbox.addWidget(plot)

            group.setLayout(vbox)
            self.road_layout.addWidget(group)

            self.plot_widgets[r] = plot

        self.road_layout.addStretch()

    # ====================== REDRAW BAR CHART ======================
    def redraw(self):
        for r in self.roads:
            if r not in self.data or r not in self.plot_widgets:
                continue  # skip until ready

            d = self.data[r]
            plt = self.plot_widgets[r]

            plt.clear()

            labels = ["Normal", "Final", "Average", "Max"]
            values = [d["normal"], d["final"], d["avg"], d["max"]]

            y = np.arange(len(values))

            bars = pg.BarGraphItem(
                x0=np.zeros(len(values)),
                x1=values,
                y=y,
                height=0.6,
                brushes=["r", "b", "y", "g"]
            )
            plt.addItem(bars)

            ticks = [(i, labels[i]) for i in range(len(labels))]
            plt.getAxis("left").setTicks([ticks])

            xmax = max(values) * 1.3 + 1
            plt.setXRange(0, xmax)

            plt.setTitle(f"Road {r}: Normal vs Adaptive Timing")


def main():
    app = QtWidgets.QApplication(sys.argv)
    ui = TimingUI()
    ui.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
