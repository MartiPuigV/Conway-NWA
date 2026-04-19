# Conway-NWA

<<<<<<< HEAD
Conway's Game of Life as a Numworks App (NWA), version 1.2.3
=======
Conway's Game of Life as a Numworks App (NWA), version 1.2.2
>>>>>>> 81c2681c83c70720697faa0db67fa8241058da3e

The NWA binary is not provided here yet. You can compile it yourself by following
the instructions at Nwagyu (link below) on how to setup your environment, then running

`make run`

The `include/eadk.h` file is not needed to compile, nwlink should handle that i believe.
I still have it to peek at function prototypes and constants.

The img-tool.py is a python script to turn a black and white image into
a conway pattern file that you can later load to your calculator using
[this](https://yaya-cout.github.io/Numworks-connector/#/) handy tool.
The resulting file should have a name like patternD.cwp, with D being a
digit 0 - 9. You can change the digit to have up to 10 patterns.

**[!] Modify the script accordingly to change image and output paths [!]**

The `src` folder should contain `pattern0.cwp`, with a glider gun pattern.
You can use any digit instead of the 0 and load it onto the calculator to use.

## Controls

| **Key**               | **Action**                                                   |
| --------------------- | ------------------------------------------------------------ |
| `OK`                  | Switch between pause and running the simulation              |
| `Arrows`              | Move the cursor around                                       |
| `Toolbox`             | Draw cell under cursor                                       |
| `Backspace`           | Erase cell under cursor.                                     |
| `Shift`               | Select area to copy (can later be pasted)                    |
| `Ans`                 | Paste copied pattern at your cursor position                 |
| `+` & `-`             | Increase/decrease frame duration                             |
| `÷`                   | Toggles strict/transparent pasting (details below)           |
| `Alpha`               | Cycles between the 3 color palettes (see below)              |
| `×`                   | Copies the entire screen as a pattern                        |
| `(` & `)`             | Cycle through 4 different resolutions (see below!)           |
| `EXE`                 | Save current configuration (palette, frame time, ...)        |
| `Back`                | Step one iteration of the simulation                         |
| `Shift` + `Backspace` | Kill all cells                                               |
| `Ln`                  | Toggle custom pixel font                                     |
<<<<<<< HEAD
| `e^x`                 | Toggle wrap on borders                                       |
=======
>>>>>>> 81c2681c83c70720697faa0db67fa8241058da3e

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

**IMPORTANT**:: For resolution changes to apply, you must change resolution, save configuration changes with `EXE`, then quit
and open the app again. The grid is created when the app opens, and needs to be reopened each time you want to change the
resolution. Future updates might circumvent this flaw.

Changes how many pixels wide a cell is. The available resolutions as of 1.2.0 are 1, 2, 4, 5 and 8 pixel wide squares for a cell.

### Custom pixel font

The pixel font is a 4x4 font containing the basic characters needed. It's a mix of two fonts, both can be found below in Aknowledgements.
The spritesheet can be found in `resources/font.png`, and is of course free to use.

<<<<<<< HEAD
### Wrapping

If enabled, wrapping prevents moving structures like gliders from dying when they encounter a border. They will reappear on the other side.
Wrapping works both horizontally and vertically.

=======
>>>>>>> 81c2681c83c70720697faa0db67fa8241058da3e
## Future updates and planned fixes

Ranked by priority

- Add panel to view all settings and savefiles
- Add ctrl+z (undo)
- Minor speed improvements
- Add tiny animations
- Add cell fade / prediction

## Aknowledgements

Thanks to [Yaya-Cout](https://github.com/Yaya-Cout) for creating the amazing nwagyu website and the
storage library (here, `src/storage.c` and `include/storage.h`).

Thanks to anyone who contributed NWA's and inspired me to do this. I tried near all of them and each one
amazes me more than the previous. Go check them out at [Nwagyu](https://yaya-cout.github.io/Nwagyu/).

Thanks to an anonymous font submitter, for some characters of the font.
Font is [here](https://fontsgeek.com/pixel-4x4-font)

Thanks to `cheeseslope` on fontstruct for most characters of the font.
Font is [here](https://fontstruct.com/fontstructions/show/1736685/bjg-pixel-brandon-james-greer)

