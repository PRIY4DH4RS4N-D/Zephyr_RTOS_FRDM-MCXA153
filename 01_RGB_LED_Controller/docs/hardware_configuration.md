# Hardware configuration

## RGB connections

| Channel | MCU signal | GPIO peripheral | Pin number | Light on | Light off |
| --- | --- | --- | ---: | ---: | ---: |
| Red | P3_12 | GPIO3 | 12 | 0 | 1 |
| Green | P3_13 | GPIO3 | 13 | 0 | 1 |
| Blue | P3_0 | GPIO3 | 0 | 0 | 1 |

The mapping is recorded in the board header and the MCUXpresso pin configuration. All channels are active low, so the GPIO output sinks current when illuminated. `RGB_SetColor(1, 0, 0)` therefore writes 0 to red and 1 to green and blue.

`BOARD_InitBootPins()` invokes the generated `BOARD_InitPins()` in `frdmmcxa153/board/pin_mux.c`. It enables and releases GPIO3 and PORT3, selects GPIO mux for P3_0, P3_12, and P3_13, then initializes the three outputs. The generated pin file initially writes low to each LED; `main()` subsequently configures the outputs high and calls `RGB_SetColor(0, 0, 0)`. A brief startup illumination before `main()` switches them off is possible and is not timed by this firmware.

The original exported example contained another `pin_mux.c` under `frdmmcxa153/led_blinky/` that configured red only. The published build selects the existing three-channel generated board pin file matching `led_blinky.mex`. No external RGB wiring is required.
