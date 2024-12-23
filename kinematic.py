import math
import serial
import time
import tkinter as tk
from tkinter import messagebox
import threading

class CableLengthController:
    def __init__(self, serial_port_name="COM9", baud_rate=57600):
        self.serial_port_name = serial_port_name
        self.baud_rate = baud_rate
        self.serial_port = None
        
        self.x_now = 70
        self.y_now = 54
        self.is_moving = False
        self.step_size=1.0
        self.height = 115
        self.width = 136
        self.origin_points = [(0, 0), (0, self.height), (self.width, self.height), (self.width, 0)] 
        self.width_square = 26.6
        self.height_square = 20.2
        self.square_half = (self.width_square / 2.0, self.height_square / 2.0)
        self.x_step=10
        self.y_step=40
        self.x_limit=75

        self.connect_serial()

    def connect_serial(self):
        try:
            self.serial_port = serial.Serial(self.serial_port_name, self.baud_rate, timeout=1)
            print(f"Connected to {self.serial_port_name} successfully.")
        except serial.SerialException as e:
            print(f"Error connecting to serial port: {e}")

   
    def move_zigzag(self, update_callback=None):
        self.is_moving = True
        direction = 1
        x_current = self.x_now
        x_step = self.x_step
        count=0
        x_start=self.x_now
        y_start=self.y_now
        while self.is_moving and count<=2:
            # Move along Y
            count+=1
            y_target = self.y_now + direction * self.y_step
            self.move_to_position(x_current, y_target, update_callback)
            print(self.y_now)
            # Cập nhật giao diện sau khi di chuyển dọc Y

            update_callback(self.x_now, self.y_now)

            # Move along X
            x_current += x_step
            # if x_current > self.x_limit or x_current < 0:
            #     # x_step = -x_step
            #     # direction *= -1  # Reverse Y direction
            #     # if abs(x_current - self.x_limit) < self.step_size:
            #     #     x_current = self.x_limit
            #     self.is_moving=False
            #     break
            
              
            self.move_to_position(x_current, y_target, update_callback)
            direction *= -1
            # Cập nhật giao diện sau khi di chuyển dọc X
            update_callback(self.x_now, self.y_now)
        self.move_to_position(x_start, y_start, update_callback)
    

    def send_data_to_serial(self, data):
        length_to_step = 100.0
        formatted_data = f"{int(data[0]*length_to_step)}/{int(data[1]*length_to_step)}*{int(data[2]*100)}${int(data[3]*length_to_step)};"
        print(f"Sending data: {formatted_data}")
        if self.serial_port:
            self.serial_port.write(formatted_data.encode())
    def calculate_cable_lengths(self, x, y):
        new_xy_center = (x, y)
        new_square_points = [
            (new_xy_center[0] - self.square_half[0], new_xy_center[1] - self.square_half[1]),
            (new_xy_center[0] - self.square_half[0], new_xy_center[1] + self.square_half[1]),
            (new_xy_center[0] + self.square_half[0], new_xy_center[1] + self.square_half[1]),
            (new_xy_center[0] + self.square_half[0], new_xy_center[1] - self.square_half[1])
        ]

        length_new = []
        for i in range(4):
            length_now = math.sqrt(
                (new_square_points[i][0] - self.origin_points[i][0])**2 + 
                (new_square_points[i][1] - self.origin_points[i][1])**2
            )
            length_new.append(length_now)

        return length_new
    def get_new_length(self, s0, s1):
        return [s1[i] - s0[i] for i in range(4)]
    def move_to_position(self, x_target, y_target, steps=80, update_callback=None):
        dx = self.step_size if x_target > self.x_now else -self.step_size
        dy = self.step_size if y_target > self.y_now else -self.step_size

        steps_x = abs(x_target - self.x_now) / self.step_size
        steps_y = abs(y_target - self.y_now) / self.step_size
        steps = int(max(steps_x, steps_y))
        total_length=[0,0,0,0]
        self.is_moving = True

        for _ in range(steps):
            if not self.is_moving:
                break

            if abs(self.x_now - x_target) < self.step_size:
                self.x_now = x_target
            else:
                self.x_now += dx

            if abs(self.y_now - y_target) < self.step_size:
                self.y_now = y_target
            else:
                self.y_now += dy

            cable_length_now = self.calculate_cable_lengths(self.x_now, self.y_now)

            if _ == 0:
                cable_length_first = cable_length_now
            new_lengths = self.get_new_length(cable_length_first, cable_length_now)
            cable_length_first = cable_length_now
            for i in range(0,4):
                total_length[i]+=new_lengths[i]
            self.send_data_to_serial(new_lengths)

            if update_callback:
                update_callback(self.x_now, self.y_now)

            if self.serial_port:
                signal=""
                while True:
                    signal = self.serial_port.readline().decode()
                    print(signal)
                    if signal.index("done")!=-1:
                        # print("OO")
                        time.sleep(0.01)
                        break
            else:
                time.sleep(0.05)
        print(total_length)
        # self.is_moving = False

    def stop_movement(self):
        self.is_moving = False

class CableApp:
    def __init__(self, master, controller):
        self.master = master
        self.controller = controller

        self.master.title("Cable Length Controller")

        self.label_current = tk.Label(master, text=f"Current Position: ({controller.x_now:.2f}, {controller.y_now:.2f})")
        self.label_current.grid(row=0, column=0, columnspan=2, pady=5)

        tk.Label(master, text="Start X:").grid(row=1, column=0, pady=5)
        self.entry_start_x = tk.Entry(master)
        self.entry_start_x.grid(row=1, column=1, pady=5)
        self.entry_start_x.insert(0, f"{controller.x_now}")

        tk.Label(master, text="Start Y:").grid(row=2, column=0, pady=5)
        self.entry_start_y = tk.Entry(master)
        self.entry_start_y.grid(row=2, column=1, pady=5)
        self.entry_start_y.insert(0, f"{controller.y_now}")

        tk.Label(master, text="Target X:").grid(row=3, column=0, pady=5)
        self.entry_x = tk.Entry(master)
        self.entry_x.grid(row=3, column=1, pady=5)

        tk.Label(master, text="Target Y:").grid(row=4, column=0, pady=5)
        self.entry_y = tk.Entry(master)
        self.entry_y.grid(row=4, column=1, pady=5)

        self.start_button = tk.Button(master, text="Set Start", command=self.set_start_position)
        self.start_button.grid(row=5, column=0, pady=10)

        self.move_button = tk.Button(master, text="Move", command=self.start_movement)
        self.move_button.grid(row=5, column=1, pady=10)
        self.zigzag_button = tk.Button(master, text="Start Zigzag", command=self.start_zigzag)
        self.zigzag_button.grid(row=7, column=0, pady=10)
        self.stop_button = tk.Button(master, text="Stop", command=self.controller.stop_movement)
        self.stop_button.grid(row=6, column=0, columnspan=2, pady=10)

    def set_start_position(self):
        try:
            start_x = float(self.entry_start_x.get())
            start_y = float(self.entry_start_y.get())
            self.controller.x_now = start_x
            self.controller.y_now = start_y
            self.update_current_position(start_x, start_y)
        except ValueError:
            messagebox.showerror("Error", "Invalid input for start position.")

    def start_movement(self):
        try:
            x = float(self.entry_x.get())
            y = float(self.entry_y.get())
            thread = threading.Thread(target=self.controller.move_to_position, args=(x, y, 60, self.update_current_position))
            thread.start()
        except ValueError:
            messagebox.showerror("Error", "Invalid input. Please enter numeric values for X and Y.")
    def start_zigzag(self):
        try:
            thread = threading.Thread(target=self.controller.move_zigzag, args=(self.update_current_position,))
            thread.start()
        except ValueError:
            messagebox.showerror("Error", "Invalid input. Please enter numeric values.")
    def update_current_position(self, x, y):
        self.label_current.config(text=f"Current Position: ({x:.2f}, {y:.2f})")

if __name__ == "__main__":
    controller = CableLengthController()

    root = tk.Tk()
    app = CableApp(root, controller)

    root.mainloop()

    if controller.serial_port:
        controller.serial_port.close()
