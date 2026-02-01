# Conway-NWA

Conway's Game of Life as a Numworks App (NWA)

The NWA binary is not provided here yet. You can compile it yourself by following
the instructions at Nwagyu (link below) on how to setup your environment, then running

`make run`

The `headers/eadk.h` file is not needed to compile, nwlink should handle that i believe.
I still have it to peek at function prototypes and constants.

The img-tool.py is a python script to turn a black and white image into
a conway pattern file (intended to be used as external data for the NWA)

**[!] Modify the script accordingly to change image and output paths [!]**

The `src` folder contains an `input.txt` file, with a glider gun pattern.
To let the NWA know to use external data, uncomment line(s) in `src/main.c`
(Should say "Optional: ...")

## Controls

|**Key**     |**Action**                                                   |
| ---------- | ----------------------------------------------------------- |
|`OK`        | Switch between pause (edit mode) and running the simulation |
|`Arrows`    | Move the cursor around (in edit mode)                       |
|`Toolbox`   | Draw cell under cursor (in edit mode)                       |
|`Backspace` | Erase cell under cursor (in edit mode)                      |
|`Shift`     | Select area to copy (can later be pasted)                   |
|`Ans`       | Paste copied pattern at your cursor position                |
|`+` & `-`   | Increase/decrease frame duration                            |
|`÷`         | Toggles strict/transparent pasting (details below)          |
|`Alpha`     | Cycles between the 3 color palettes (see below)             |
|`×`         | Copies the entire screen as a pattern                       |
|`(` & `)`   | Cycle through 4 different resolutions (see below!)          |
|`EXE`       | Save current configuration (palette, frame time, ...)       |

## Details

### Strict vs. Transparent pasting:

- Strict pasting will paste anything the original pattern contains, including dead cells. This might overwrite
live cells with dead ones.

- Transparent pasting, as its name suggest, acts as a transparent "image", and will
only paste live cells. In transparent mode, selecting too large an area with too many dead cells is not a
problem, as pasting will not overwrite a large rectangle with dead cells.

### Color palettes:

- White
- Green
- Peach / Beige

Green and Peach colors come from [here](https://www.deviantart.com/advancedfan2020/art/Game-Boy-Palette-Set-Color-HEX-Part-12-920496174)

### Resolution:

IMPORTANT:: For resolution changes to apply, you must change resolution, save configuration changes with `EXE`, then quit
and open the app again. The grid is created when the app opens, and needs to be reopened each time you want to change the
resolution. Future updates might circumvent this flaw.

Changes how many pixels wide a cell is. The available resolutions as of 1.1.0 are 2, 4, 5 and 8 pixel wide squares for a cell.
A 1:1 pixel:cell ratio was doable in older versions, but newer versions fall short of RAM for that luxury. Don't worry about
over- or undershooting those values, as it will simply wrap around.

## Future updates and planned fixes

- Minor speed and major memory improvements
- Allow multiple pattern save slots (0-9)
- Allow step by step simulation (maybe step back too)
- Add icon

## Aknowledgements

Thanks to [Yaya-Cout](https://github.com/Yaya-Cout) for creating the amazing nwagyu website and the
storage library (here, src/storage.c and headers/storage.h).

Thanks to anyone who contributed NWA's and inspired me to do this. I tried near all of them and each one
amazes me more than the previous. Go check them out at [Nwagyu](https://yaya-cout.github.io/Nwagyu/).

