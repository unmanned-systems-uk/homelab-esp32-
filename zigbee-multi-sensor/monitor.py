#!/usr/bin/env python3
"""
ESP32 Multi-Sensor Monitor
Displays BH1750, DS18B20, and DHT11 sensor data in real-time
"""

import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import re
import sys

class SensorMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Zigbee Multi-Sensor Monitor")
        self.root.geometry("550x650")
        self.root.resizable(False, False)

        self.serial_port = None
        self.running = False

        # Sensor data
        self.light_lux = tk.StringVar(value="-- lux")
        self.outdoor_temp = tk.StringVar(value="-- °C")
        self.indoor_temp = tk.StringVar(value="-- °C")
        self.humidity = tk.StringVar(value="-- %")

        # Zigbee diagnostics
        self.zigbee_connected = tk.StringVar(value="Unknown")
        self.zigbee_channel = tk.StringVar(value="--")
        self.zigbee_short_addr = tk.StringVar(value="----")
        self.zigbee_pan_id = tk.StringVar(value="--:--:--:--:--:--:--:--")

        self.setup_ui()
        self.auto_connect()

    def setup_ui(self):
        # Title
        title = tk.Label(self.root, text="🌡️ ESP32 Multi-Sensor Monitor",
                        font=("Arial", 16, "bold"), pady=10)
        title.pack()

        # Main frame for sensor displays
        main_frame = tk.Frame(self.root, padx=20, pady=10)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # BH1750 Light Sensor
        self.create_sensor_box(main_frame, "💡 BH1750 Light",
                               self.light_lux, 0, "#FFE5B4")

        # DS18B20 Outdoor Temperature
        self.create_sensor_box(main_frame, "🌡️ DS18B20 Outdoor",
                               self.outdoor_temp, 1, "#B4D7FF")

        # DHT11 Indoor Temperature
        self.create_sensor_box(main_frame, "🏠 DHT11 Indoor Temp",
                               self.indoor_temp, 2, "#FFB4B4")

        # DHT11 Humidity
        self.create_sensor_box(main_frame, "💧 DHT11 Humidity",
                               self.humidity, 3, "#B4FFB4")

        # Zigbee Diagnostics Section
        zigbee_frame = tk.LabelFrame(main_frame, text="📡 Zigbee Network Status",
                                      font=("Arial", 11, "bold"),
                                      relief=tk.RIDGE, borderwidth=2,
                                      bg="#E8E8FF", padx=10, pady=8)
        zigbee_frame.grid(row=4, column=0, sticky="ew", pady=10)

        # Connection Status
        conn_frame = tk.Frame(zigbee_frame, bg="#E8E8FF")
        conn_frame.pack(fill=tk.X, pady=2)
        tk.Label(conn_frame, text="Connected:", font=("Arial", 9, "bold"),
                bg="#E8E8FF", width=12, anchor=tk.W).pack(side=tk.LEFT)
        tk.Label(conn_frame, textvariable=self.zigbee_connected,
                font=("Arial", 9), bg="#E8E8FF", anchor=tk.W).pack(side=tk.LEFT)

        # Channel
        chan_frame = tk.Frame(zigbee_frame, bg="#E8E8FF")
        chan_frame.pack(fill=tk.X, pady=2)
        tk.Label(chan_frame, text="Channel:", font=("Arial", 9, "bold"),
                bg="#E8E8FF", width=12, anchor=tk.W).pack(side=tk.LEFT)
        tk.Label(chan_frame, textvariable=self.zigbee_channel,
                font=("Arial", 9), bg="#E8E8FF", anchor=tk.W).pack(side=tk.LEFT)

        # Short Address
        addr_frame = tk.Frame(zigbee_frame, bg="#E8E8FF")
        addr_frame.pack(fill=tk.X, pady=2)
        tk.Label(addr_frame, text="Short Addr:", font=("Arial", 9, "bold"),
                bg="#E8E8FF", width=12, anchor=tk.W).pack(side=tk.LEFT)
        tk.Label(addr_frame, textvariable=self.zigbee_short_addr,
                font=("Arial", 9), bg="#E8E8FF", anchor=tk.W).pack(side=tk.LEFT)

        # PAN ID
        pan_frame = tk.Frame(zigbee_frame, bg="#E8E8FF")
        pan_frame.pack(fill=tk.X, pady=2)
        tk.Label(pan_frame, text="PAN ID:", font=("Arial", 9, "bold"),
                bg="#E8E8FF", width=12, anchor=tk.W).pack(side=tk.LEFT)
        tk.Label(pan_frame, textvariable=self.zigbee_pan_id,
                font=("Arial", 8), bg="#E8E8FF", anchor=tk.W).pack(side=tk.LEFT)

        # Status bar
        self.status_var = tk.StringVar(value="Disconnected")
        status_bar = tk.Label(self.root, textvariable=self.status_var,
                             relief=tk.SUNKEN, anchor=tk.W,
                             font=("Arial", 9), bg="#f0f0f0", pady=5)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

        # Control buttons
        button_frame = tk.Frame(self.root)
        button_frame.pack(pady=10)

        self.connect_btn = tk.Button(button_frame, text="Connect",
                                     command=self.connect, width=10)
        self.connect_btn.pack(side=tk.LEFT, padx=5)

        self.disconnect_btn = tk.Button(button_frame, text="Disconnect",
                                        command=self.disconnect, width=10,
                                        state=tk.DISABLED)
        self.disconnect_btn.pack(side=tk.LEFT, padx=5)

    def create_sensor_box(self, parent, label_text, text_var, row, color):
        frame = tk.Frame(parent, relief=tk.RIDGE, borderwidth=2,
                        bg=color, padx=10, pady=8)
        frame.grid(row=row, column=0, sticky="ew", pady=5)
        parent.grid_columnconfigure(0, weight=1)

        label = tk.Label(frame, text=label_text, font=("Arial", 11, "bold"),
                        bg=color)
        label.pack(anchor=tk.W)

        value = tk.Label(frame, textvariable=text_var,
                        font=("Arial", 20, "bold"), bg=color)
        value.pack(anchor=tk.CENTER, pady=5)

    def auto_connect(self):
        """Automatically find and connect to ESP32"""
        ports = serial.tools.list_ports.comports()
        esp32_port = None

        for port in ports:
            desc = port.description.lower()
            mfr = (port.manufacturer or "").lower()

            # Look for ESP32 devices:
            # 1. Built-in USB-JTAG (ESP32-C6, ESP32-S3, etc.)
            # 2. Common USB-serial chips (CH340, CP210x, FTDI)
            if any(match in desc or match in mfr for match in
                   ['espressif', 'jtag', 'cp210', 'ch340', 'ch9102', 'ftdi', 'usb serial']):
                esp32_port = port.device
                break

        if esp32_port:
            self.connect(esp32_port)
        else:
            self.status_var.set("No ESP32 found. Click Connect to select port.")

    def connect(self, port=None):
        if self.running:
            return

        if not port:
            # Manual port selection
            ports = [p.device for p in serial.tools.list_ports.comports()]
            if not ports:
                messagebox.showerror("Error", "No serial ports found!")
                return

            port = ports[0]  # Use first available port
            if len(ports) > 1:
                # Could add a selection dialog here
                pass

        try:
            self.serial_port = serial.Serial(port, 115200, timeout=1)
            self.running = True
            self.status_var.set(f"Connected to {port}")

            # Start reading thread
            self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
            self.read_thread.start()

            self.connect_btn.config(state=tk.DISABLED)
            self.disconnect_btn.config(state=tk.NORMAL)

        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Failed to connect: {e}")
            self.status_var.set("Connection failed")

    def disconnect(self):
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()

        self.status_var.set("Disconnected")
        self.connect_btn.config(state=tk.NORMAL)
        self.disconnect_btn.config(state=tk.DISABLED)

    def read_serial(self):
        """Read and parse serial data in background thread"""
        while self.running:
            try:
                if self.serial_port and self.serial_port.is_open:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()

                    if line:
                        self.parse_line(line)

            except serial.SerialException:
                self.running = False
                self.root.after(0, self.disconnect)
                break
            except Exception as e:
                print(f"Error reading serial: {e}")

    def parse_line(self, line):
        """Parse sensor data and Zigbee diagnostics from ESP32 output"""
        try:
            # BH1750: Light:    91.7 lux
            if "BH1750" in line and "Light:" in line:
                match = re.search(r'Light:\s+([\d.]+)\s+lux', line)
                if match:
                    self.light_lux.set(f"{match.group(1)} lux")

            # DS18B20: Temp:   21.38 °C  (70.47 °F)  [Outdoor]
            elif "DS18B20" in line and "Temp:" in line:
                match = re.search(r'Temp:\s+([\d.]+)\s+°C', line)
                if match:
                    temp = float(match.group(1))
                    temp_f = (temp * 9.0/5.0) + 32.0
                    self.outdoor_temp.set(f"{temp:.1f} °C ({temp_f:.1f} °F)")

            # DHT11: Temp:    30.0 °C  (86.0 °F)  [Indoor]
            elif "DHT11" in line and "Temp:" in line:
                match = re.search(r'Temp:\s+([\d.]+)\s+°C', line)
                if match:
                    temp = float(match.group(1))
                    temp_f = (temp * 9.0/5.0) + 32.0
                    self.indoor_temp.set(f"{temp:.1f} °C ({temp_f:.1f} °F)")

            # DHT11: Humid:   32.0 %
            elif "DHT11" in line and "Humid:" in line:
                match = re.search(r'Humid:\s+([\d.]+)\s+%', line)
                if match:
                    self.humidity.set(f"{match.group(1)} %")

            # Zigbee Diagnostics
            # Connected:    YES or Connected:    NO
            elif "Connected:" in line and ("YES" in line or "NO" in line):
                if "YES" in line:
                    self.zigbee_connected.set("✓ YES")
                else:
                    self.zigbee_connected.set("✗ NO (searching...)")

            # Channel:      11
            elif "Channel:" in line:
                match = re.search(r'Channel:\s+(\d+)', line)
                if match:
                    self.zigbee_channel.set(match.group(1))

            # Short Addr:   0x1234
            elif "Short Addr:" in line:
                match = re.search(r'Short Addr:\s+(0x[0-9A-Fa-f]+)', line)
                if match:
                    self.zigbee_short_addr.set(match.group(1))

            # PAN ID:       dd:dd:dd:dd:dd:dd:dd:dd
            elif "PAN ID:" in line:
                match = re.search(r'PAN ID:\s+([\da-fA-F:]+)', line)
                if match:
                    self.zigbee_pan_id.set(match.group(1))

        except Exception as e:
            print(f"Error parsing line: {e}")

    def on_closing(self):
        self.disconnect()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = SensorMonitor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
