# GroundTX UI prototype

GroundTX is an experimental surface-radio interface for the RadioMaster MT12.
The current `0.2 demo` is an EdgeTX Lua tool: it demonstrates the navigation,
surface setup screens, and mix-preset workflows without changing the active
model.

## Install on an MT12

1. Copy `SCRIPTS/TOOLS/GroundTX.lua` to the same path on the radio SD card.
2. Start the radio normally.
3. Open **SYS**, select **Tools**, then launch **GroundTX**.

The prototype targets the MT12's 128x64 monochrome display.

## Controls

- Rotate the wheel to move through rows or change the selected value.
- Press the wheel to open a screen or start/finish editing a value.
- Press **RTN** to cancel an edit, return to the previous screen, or exit from
  the dashboard.

All settings currently live only in Lua memory and are discarded when the tool
closes. The `DEMO` indicator is a reminder that the script does not write to
the active EdgeTX model yet.

## Prototype coverage

- Surface-focused dashboard
- Plain-language main menu
- Steering, throttle, brake, and ABS setup concepts
- Mix preset browser
- Crawler 4WS setup
- Front-only, opposite, crab, rear-only, and selectable steering modes
- Front/rear channel, rear rate, rear reverse, and mode-control selection
- Diagram-based 4WS preview
- Dual ESC/dig, tank steering, lighting, and winch concept pages
- Safe model-template selector
- Radio-preference and advanced-settings concepts
- Built-in project and feedback page

## Share the prototype

The entire runnable demo is the single `GroundTX.lua` file. Friends can copy it
to `SCRIPTS/TOOLS/` without installing firmware or changing their models. Tell
testers that every screen marked `DEMO` is temporary and nothing is saved.

Useful feedback questions:

1. Could you find steering rate and endpoints without instructions?
2. Does the 4WS terminology match what crawler drivers actually call the modes?
3. Which setup still feels like it requires EdgeTX knowledge?
4. Which mix preset should be implemented after 4WS?
5. What information must always appear on the dashboard?

## Test checklist

- Every row should be readable without horizontal clipping.
- Navigation should feel natural using only the wheel and RTN button.
- Editing should be visually obvious and RTN should cancel the change.
- A new surface user should be able to configure the 4WS page without knowing
  EdgeTX's input, mix, or output terminology.
- The steering preview should make each mode understandable without a manual.

The next stage will refine the interface from real-world feedback, then
translate an accepted setup into EdgeTX model mixes with a confirmation screen
and rollback protection before any model data is changed.
