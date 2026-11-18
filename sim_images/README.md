# Simulator photos

Put `.jpg` photos here to try the Claude brain without any hardware:

1. Hold your phone about **10 cm above the floor**, pointing forward (like the rover's camera).
2. Take a photo, "move" (turn 45° or step forward), take another one, and so on. 5–15 photos is plenty.
3. Name them so they sort in order: `01.jpg`, `02.jpg`, …
4. Run:
   ```bash
   python -m rover mission --hardware sim "Explore the room and find a shoe"
   ```

The simulated camera shows the photos in order, whatever Claude decides, so this tests how Claude *sees and reasons*, not real navigation.
