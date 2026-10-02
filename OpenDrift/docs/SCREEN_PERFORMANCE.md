# AMOLED rendering performance

The swipe path previously recomposed and rotated every pixel each frame, using
large-stride framebuffer writes in PSRAM. Release animations used a fixed six
frames, and the drag preview added a pacing delay after each completed push.
Live Radio/Steering pages were deliberately refreshed only every 250 ms.

The updated path:

- Transposes the two page overlays and stationary background once per swipe.
- Merges cached physical scanlines sequentially during movement, preserving
  transparency and background-aware panel tint.
- Uses elapsed-time easing, with approximately 16.7 ms minimum frame spacing.
  A slow transfer counts toward that interval rather than adding another full
  delay afterward. This targets 60 Hz pacing, not guaranteed 60 FPS.
- Updates live Radio/Steering values every 50 ms (20 Hz maximum); other idle
  status pages keep their existing lower update rate.
- Leaves the panel SPI clock and independent controller task unchanged.

The swipe cache is allocated lazily in PSRAM and occupies 766,080 bytes. It is
retained for later swipes. If allocation fails, the previous pixel compositor
remains available, with the new elapsed-time animation timing.

## Hardware verification

Test dragging slowly, flicking quickly, cancelling a swipe, wrapping between
first/last pages, and reversing direction mid-gesture. Check default/custom
backgrounds, light/dark text, and both display orientations. Verify that the
background remains stationary and that final page dots match the selected page.

At swipe completion, serial output reports:

```text
UI swipe: ... frames, ... fps, compose ... ms, transfer ... ms, cache=ON
```

FPS is the average delivered transition frames across the gesture and its
release, so holding a finger still lowers the average. Composition and transfer
times are per-frame averages. These measurements do not include page drawing
and initial cache construction. Use an uninterrupted swipe when comparing runs.
If animation remains choppy, send several reports along with whether a custom
background is selected and whether the issue occurs during dragging or release.

Also compare the control timing capture with/without swiping. Faster UI rendering
must not be treated as evidence that controller deadlines are still met.
