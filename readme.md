# scs-sdk-controller
This dll file is built on top of the SCS SDK that is provided by SCS Software for Euro Truck Simulator 2 and American Truck Simulator. It is meant to be used by other programs to control the game.

A prebuilt SDK is provided at ```scs_sdk_1_14/examples/input_semantical/x64/Debug/input_semantical.dll```

The program will first create, and then listen to changes on the following shared memory file: ```"Local\\SCSControls"```

Available controls are:
```
Name, Index, Type, Control Name In File
In total there are 4 axis and 38 buttons

Steering, 0, float, steering
Acceleration, 1, float, aforward
Braking, 2, float, abackward
Clutch, 3, float, clutch
Pause Game, 4, bool,
Parking Brake, 5, bool, parkingbrake
Wipers, 6, bool, wipers
Cruise Control, 7, bool, cruiectrl
Cruise Control Increase, 8, bool, cruiectrlinc
Cruise Control Decrease, 9, bool, cruiectrldec
Cruise Control Reset, 10, bool, cruiectrlres
Lights, 11, bool, light
High Beams, 12, bool, hblight
Left Blinker, 13, bool, lblinker
Right Blinker, 14, bool, rblinker
Quickpark, 15, bool, quickpark
Drive(Gear), 16, bool, drive
Reverse(Gear), 17, bool, reverse
Cycle Zoom(map ? ), 18, bool, cycl_zoom
Reset Trip, 19, bool, tripreset
Rear Wipers, 20, bool, wipersback
Wiper LVL 0, 21, bool, wipers0
Wiper LVL 1, 22, bool, wipers1
Wiper LVL 2, 23, bool, wipers2
Wiper LVL 3, 24, bool, wipers3
Wiper LVL 4, 25, bool, wipers4
Horn, 26, bool, horn
Airhorn, 27, bool, airhorn
Light Horn, 28, bool, lighthorn
Camera 1, 29, bool, cam1
Camera 2, 30, bool, cam2
Camera 3, 31, bool, cam3
Camera 4, 32, bool, cam4
Camera 5, 33, bool, cam5
Camera 6, 34, bool, cam6
Camera 7, 35, bool, cam7
Camera 8, 36, bool, cam8
Zoom Map In, 37, bool, mapzoom_in
Zoom Map Out, 38, bool, mapzoom_out
ACC Mode, 39, bool, accmode
Show Mirrors, 40, bool, showmirrors
Hazard Lights, 41, bool, flasher4way
```

If you need additional controls then please contact me on github, discord @Tumppi066 or email me at contact@tumppi066.fi

Example python code to control from [my other program](https://github.com/Tumppi066/Euro-Truck-Simulator-2-Lane-Assist) (some code is cut out for clarity):
```python
import mmap
mmName = r"Local\SCSControls"
floatCount = 4
floatSize = 4
boolCount = 15
boolSize = 1
size = floatCount * floatSize + boolCount * boolSize

def plugin(data): # Called each frame
    buf = None
    try:
        buf = mmap.mmap(0, size, mmName)
    except:
        return data
    
    if buf == None:
        return data

    # Code to get the values from the data variable
    # ...
    
    # Write three floats to memory
    # ffff = 4 floats
    # 15? = 15 bools
    # Values have to be added in the same order!
    buf[:] = struct.pack('ffff15?', steering, acceleration, brake, clutch,
                         pause, parkingbrake, wipers, cruiectrl, cruiectrlinc, cruiectrldec, 
                         cruiectrlres, light, hblight, lblinker, rblinker, quickpark, drive, 
                         reverse, cycl_zoom)

    return data
```

# Build instructions
1. Download VS 2022 with C++ support (v143)
2. Open the solution file ```scs_sdk_1_14/examples/input_semantical/input_semantical.sln```
3. Build the solution. WITH THE x64 CONFIG!
4. The dll file will be in ```scs_sdk_1_14/examples/input_semantical/x64/Debug/input_semantical.dll```
