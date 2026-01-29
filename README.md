# Conway-NWA

Conway's game of life as a Numworks App

NWA is not provided here. You can compile it yourself by following the instructions
at nwagyu (yaya-cout.github.io/Nwagyu) on how to setup your environment, then running

'make run'

The img-tool.py is a python script to turn a black and white image into
a conway pattern file (intended to be used as external data for the NWA)

!Modify the script accordingly to change image and output paths!

The src/ folder contains an input.txt file, with a glider gun pattern.
To let the NWA know to use external data, uncomment 2 lines in src/main.c
(Should say "Optional: ..." around those lines)

# Controls

Use OK to switch between pause (menu/edit mode) and running the simulation.

When in edit mode, use the arrows to control a pink cursor. Toolbox (to the
left of backspace) and backspace can be used to draw / erase cells, in that
order.

By pressing shift, you place your first point of a selection rectangle. When
pressing shift a second time, the cells inside the shown rectangle will be
locally copied in a file called "pattern.cwp" on your calculator. Use the Ans
key to paste at your cursor.
Pasting too close to the right edge wraps the pattern around, and too low only draws what it can.
A prior issue where pasting too low *could* cause a reset should now be fixed.

Simulation speed can be changed with + and -.

!They do not represent the speed, but rather the time between each frame!

Increasing (pressing +) the time slows down the simulation (and pressing - speeds it up).
Pressing the division key cycles between 3 color palettes for live cells:

White (0xFFFF)
Green (0xBECA)
Peach (0xFDCF)

(Green and Peach colors come [here](https://www.deviantart.com/advancedfan2020/art/Game-Boy-Palette-Set-Color-HEX-Part-12-920496174)

# Future updates and planned fixes

- Bug fixes and minor speed improvements
- Code cleanup
- Allow for variable canvas scale (zooming)
- Allow multiple pattern save slots (0-9)
- Make color scheme also change the background color
- Allow strict or normal pasting
    Strict: Paste dead cells onto possibly live ones, and live cells
    Normal: Only paste live cells, ignore the dead ares of the pattern

# Aknowledgements

Thanks to Yaya-Cout for creating the amazing nwagyu website and the storage library,
storage.c and storage.h.

Thanks to anyone who contributed NWA's and inspired me to do this. I tried near all of them
and each one amazes me more than the previous.

