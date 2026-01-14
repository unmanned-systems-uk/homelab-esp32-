#!/usr/bin/env python3
"""
Simple ESP32 Monitor - Direct connect to /dev/ttyACM0
"""

import tkinter as tk
import serial
import threading
import re
import time

class SensorMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Multi-Sensor Monitor")
        self.root.geometry("500x480")

        self.serial_port = None
        self.running = False

        # Sensor data
        self.light_lux = tk.StringVar(value="-- lux")
        self.outdoor_temp = tk.StringVar(value="-- °C")
        self.indoor_temp = tk.StringVar(value="-- °C")
        self.humidity = tk.StringVar(value="-- %")

        self.setup_ui()

        # Auto-connect after UI is ready
        self.root.after(500, self.try_connect)

    def setup_ui(self):
        # Title
        title = tk.Label(self.root, text="🌡️ ESP32 Multi-Sensor Monitor",
                        font=("Arial", 16, "bold"), pady=10)
        title.pack()

        # Main frame
        main_frame = tk.Frame(self.root, padx=20, pady=10)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # Sensor boxes
        self.create_sensor_box(main_frame, "💡 BH1750 Light", self.light_lux, 0, "#FFE5B4")
        self.create_sensor_box(main_frame, "🌡️ DS18B20 Outdoor", self.outdoor_temp, 1, "#B4D7FF")
        self.create_sensor_box(main_frame, "🏠 DHT11 Indoor Temp", self.indoor_temp, 2, "#FFB4B4")
        self.create_sensor_box(main_frame, "💧 DHT11 Humidity", self.humidity, 3, "#B4FFB4")

        # Status bar
        self.status_var = tk.StringVar(value="Waiting for /dev/ttyACM0...")
        status_bar = tk.Label(self.root, textvariable=self.status_var,
                             relief=tk.SUNKEN, anchor=tk.W, bg="#f0f0f0", pady=5)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def create_sensor_box(self, parent, label_text, text_var, row, color):
        frame = tk.Frame(parent, relief=tk.RIDGE, borderwidth=2,
                        bg=color, padx=10, pady=8)
        frame.grid(row=row, column=0, sticky="ew", pady=5)
        parent.grid_columnconfigure(0, weight=1)

        label = tk.Label(frame, text=label_text, font=("Arial", 11, "bold"), bg=color)
        label.pack(anchor=tk.W)

        value = tk.Label(frame, textvariable=text_var,
                        font=("Arial", 20, "bold"), bg=color)
        value.pack(anchor=tk.CENTER, pady=5)

    def try_connect(self):
        """Keep trying to connect to /dev/ttyACM0"""
        if self.running:
            return

        try:
            self.serial_port = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
            self.running = True
            self.status_var.set("✓ Connected to /dev/ttyACM0")

            # Start reading thread
            threading.Thread(target=self.read_serial, daemon=True).start()

        except (serial.SerialException, FileNotFoundError) as e:
            self.status_var.set(f"Waiting for ESP32... ({str(e)[:40]})")
            # Try again in 2 seconds
            self.root.after(2000, self.try_connect)

    def read_serial(self):
        """Read and parse serial data"""
        consecutive_errors = 0

        while self.running:
            try:
                if self.serial_port and self.serial_port.is_open:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()

                    if line:
                        self.parse_line(line)
                        consecutive_errors = 0

            except serial.SerialException:
                consecutive_errors += 1
                if consecutive_errors > 5:
                    self.running = False
                    self.status_var.set("Connection lost - trying to reconnect...")
                    self.root.after(1000, self.try_connect)
                    break
                time.sleep(0.5)
            except Exception as e:
                print(f"Error: {e}")
                time.sleep(0.5)

    def parse_line(self, line):
        """Parse sensor data from ESP32 output"""
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
                    self.outdoor_temp.set(f"{temp:.1f} °C\n({temp_f:.1f} °F)")

            # DHT11: Temp:    30.0 °C  (86.0 °F)  [Indoor]
            elif "DHT11" in line and "Temp:" in line:
                match = re.search(r'Temp:\s+([\d.]+)\s+°C', line)
                if match:
                    temp = float(match.group(1))
                    temp_f = (temp * 9.0/5.0) + 32.0
                    self.indoor_temp.set(f"{temp:.1f} °C\n({temp_f:.1f} °F)")

            # DHT11: Humid:   32.0 %
            elif "DHT11" in line and "Humid:" in line:
                match = re.search(r'Humid:\s+([\d.]+)\s+%', line)
                if match:
                    self.humidity.set(f"{match.group(1)} %")

        except Exception as e:
            print(f"Parse error: {e}")

    def on_closing(self):
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = SensorMonitor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
