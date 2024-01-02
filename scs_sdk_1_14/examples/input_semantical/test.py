import socket
import struct
import time

def send_data(conn, steering, aforward, abackward):
    # Pack the floats into a binary format
    packed_data = struct.pack('fff', steering, aforward, abackward)
    
    # Send the packed data
    conn.sendall(packed_data)

# Create a socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Set timeout mode to non blocking
s.setblocking(0)

# Bind the socket to a specific address and port
s.bind(('localhost', 12345))

print('Socket server bound to port 12345')

# Listen for incoming connections
s.listen(1)

print('Socket server started')

import time

steering = -1
counter = 0
while True:
    # Accept a connection
    try:
        conn, addr = s.accept()
    except:
        continue
    
    print('Connected by', addr)
    
    try:
        while True:
            startTime = time.time()
            try:
                send_data(conn, steering, 0, 0)
            except: 
                continue
            endTime = time.time()
            steering += (endTime - startTime)
            if steering > 1:
                steering = -1
            
            if counter % 10 == 0:
                print(steering, end='\r')
                counter = 0
            else:
                counter += 1
            # time.sleep(0.1)
    except:
        print('Connection closed')
        conn.close()