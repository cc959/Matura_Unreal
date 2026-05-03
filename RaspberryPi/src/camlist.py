# camlist.py
# Lists all avaiable cameras attached to the computer
# Dependencies: pip install opencv-python
# Usage: python camlist.py

import cv2
import time
print(f"OpenCV version: {cv2.__version__}")

max_cameras = 10
avaiable = []
caps = []
for i in range(max_cameras):
    cap = cv2.VideoCapture(i, cv2.CAP_V4L2)
    
    if not cap.read()[0]:
        print(f"Camera index {i:02d} not found...")
        continue
    
    avaiable.append(i)
    caps.append(cap)
    
    print(f"Camera index {i:02d} OK!")
    time.sleep(1)


print(f"Cameras found: {avaiable}")

# Take one picture from each available camera
print("\nTaking one picture from each camera...")

for i, cap in enumerate(caps):
    # Ensure camera is open
    if not cap.isOpened():
        print(f"Camera {avaiable[i]:02d} is not open, skipping...")
        continue
    
    # Read a frame
    ret, frame = cap.read()
    
    if not ret:
        print(f"Failed to capture image from camera {avaiable[i]:02d}")
        continue
    
    # Save the image
    filename = f"camera_{avaiable[i]:02d}.jpg"
    cv2.imwrite(filename, frame)
    print(f"Saved image from camera {avaiable[i]:02d} to {filename}")

# Release all camera resources
print("\nReleasing camera resources...")
for cap in caps:
    cap.release()

print("Done!")
