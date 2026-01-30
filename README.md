# Conway-NWA

Conway's game of life as a Numworks App

NWA is not provided here yet. You can compile it yourself by following the instructions
at nwagyu (yaya-cout.github.io/Nwagyu) on how to setup your environment, then running

'make run'

The img-tool.py is a python script to turn a black and white image into
a conway pattern file (intended to be used as external data for the NWA)

! Modify the script accordingly to change image and output paths !

The src/ folder contains an input.txt file, with a glider gun pattern.
To let the NWA know to use external data, uncomment line(s) in src/main.c
(Should say "Optional: ...")

# Controls

- OK: switch between pause (menu/edit mode) and running the simulation.

When in edit mode, use the arrows to control a pink cursor. Toolbox (to the
left of backspace) and backspace can be used to draw / erase cells, in that
order.

- Shift: places your first point of a selection rectangle. When
pressing shift a second time, the cells inside the shown rectangle will be
locally copied in a file called "pattern.cwp" on your calculator.

- Ans: Paste copied pattern at your cursor position.

- [+] & [-]: Change simulation speed.

! They do not represent the speed, but rather the time between each frame !

- / (division): Toggles strict/transparent pasting. Transparent pasting only pastes
live cells, while strict will overwrite the entire selection rectangle with what the
pattern contains, even writing dead cells to the grid.

- alpha: Cycles between the 3 color palettes

White
Green
Peach / Beige

Green and Peach colors come from [here](https://www.deviantart.com/advancedfan2020/art/Game-Boy-Palette-Set-Color-HEX-Part-12-920496174)

- * (multiplication): Copies the entire screen as a pattern

- "(" & ")": Cycle through 4 different scales. Changes only apply when loading settings
at app start. 

- EXE: Save current config (Color palette, grid scale, simulation speed)
The configurations should automatically load when the app launches.

# Future updates and planned fixes

- Minor speed improvements
- Allow multiple pattern save slots (0-9)

# Aknowledgements

Thanks to Yaya-Cout for creating the amazing nwagyu website and the storage library,
storage.c and storage.h.

Thanks to anyone who contributed NWA's and inspired me to do this. I tried near all of them
and each one amazes me more than the previous.

