import sys
import subprocess

def install_and_import(package, import_name=None):
    import_name = import_name or package
    try:
        __import__(import_name)
    except ImportError:
        print(f"Installing package: {package} ...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])
        __import__(import_name)

install_and_import("pyserial", "serial")
install_and_import("numpy")
install_and_import("matplotlib")




import serial
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import re

# ---- CONFIGURE YOUR SERIAL PORT ----
SERIAL_PORT = 'COM11'  # <-- Change this to your port! (e.g., 'COM4' on Windows, '/dev/ttyUSB0' on Linux)
BAUD_RATE = 9600

# Convert degrees to radians
def deg2rad(deg):
    return deg * np.pi / 180.0

# Convert Euler angles (yaw, pitch, roll) to rotation matrix (ZYX order)
def euler_to_matrix(yaw, pitch, roll):
    # In radians
    yaw, pitch, roll = map(deg2rad, (yaw, pitch, roll))
    Rx = np.array([
        [1, 0, 0],
        [0, np.cos(roll), -np.sin(roll)],
        [0, np.sin(roll), np.cos(roll)]
    ])
    Ry = np.array([
        [np.cos(pitch), 0, np.sin(pitch)],
        [0, 1, 0],
        [-np.sin(pitch), 0, np.cos(pitch)]
    ])
    Rz = np.array([
        [np.cos(yaw), -np.sin(yaw), 0],
        [np.sin(yaw), np.cos(yaw), 0],
        [0, 0, 1]
    ])
    return Rz @ Ry @ Rx

# Visualization function
def plot_orientation(ax, R):
    # Clear last
    ax.cla()

    # Origin
    origin = np.zeros(3)

    # Sensor axes (unit vectors)
    x_axis = np.array([1, 0, 0])
    y_axis = np.array([0, 1, 0])
    z_axis = np.array([0, 0, 1])

    # Rotate axes
    x_axis_rot = R @ x_axis
    y_axis_rot = R @ y_axis
    z_axis_rot = R @ z_axis

    # Plot
    ax.quiver(*origin, *x_axis_rot, color='r', length=1, label='X')
    ax.quiver(*origin, *y_axis_rot, color='g', length=1, label='Y')
    ax.quiver(*origin, *z_axis_rot, color='b', length=1, label='Z')

    # Make it pretty
    ax.set_xlim([-1.3, 1.3])
    ax.set_ylim([-1.3, 1.3])
    ax.set_zlim([-1.3, 1.3])
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title("MPU-9250 Orientation (Pitch-Roll-Yaw)")
    ax.legend()
    plt.draw()
    plt.pause(0.01)

def parse_angles(line):
    match = re.match(r'(Pitch|Roll|Yaw):\s*([\-.\d]+)', line)
    if match:
        axis = match.group(1)
        value = float(match.group(2))
        return axis, value
    return None

def main():
    # Open serial connection
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print("Listening on", SERIAL_PORT)

    pitch = roll = yaw = 0.0

    # Set up plot
    plt.ion()
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    try:
        while True:
            line = ser.readline().decode(errors='ignore').strip()
            if not line: continue
            result = parse_angles(line)
            if result:
                axis, value = result
                if axis == 'Pitch':
                    pitch = value
                elif axis == 'Roll':
                    roll = value
                elif axis == 'Yaw':
                    yaw = value
                # Draw on every yaw update, or adjust if you prefer
                if axis == 'Yaw':
                    R = euler_to_matrix(yaw, pitch, roll)
                    plot_orientation(ax, R)
    except KeyboardInterrupt:
        ser.close()
        plt.ioff()
        plt.show()
        print("Exited gracefully.")

if __name__ == '__main__':
    main()
