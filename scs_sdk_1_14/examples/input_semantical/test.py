import mmap
import os
import struct
import time

# Open the file
mmName = r"Local\SCSControls"

# Memory map the file
print("Waiting for memory map from game...")
buf = None
while buf is None:
    buf = mmap.mmap(0, 3 * 4, "Local\SCSControls")  # 3 floats, 4 bytes each
    time.sleep(0.1)
print("Memory map received!")

steering = -1
didIncrease = False
try:
    while True:
        # Write three floats to memory
        buf[:] = struct.pack('fff', steering, 0.0, 0.0)

        # Sleep for a while to prevent high CPU usage
        time.sleep(0.01)
        if didIncrease:
            steering = steering - 0.005
        else:
            steering = steering + 0.005
        if steering > 1:
            didIncrease = True
        elif steering < -1:
            didIncrease = False
        print(steering, end='\r')
except:
    pass