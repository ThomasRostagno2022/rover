# 3D model of the rover

Two ways to look at the rover before you build it:

| File | What it is | How to open it |
|---|---|---|
| **Online viewer** | Interactive 3D model of options A, B and C. Spin it, explode it, click parts for buy links, and see a live "what Claude sees" camera view. | https://claude.ai/artifact/VqTWkRmQ2hFEWxdDdbjGNp |
| `rover-3d.html` | The same viewer, as a file in this repo | Double-click it to open in your web browser (needs internet to load the 3D library) |
| `rover.scad` | A real, parametric CAD model | Install **OpenSCAD** (free, https://openscad.org), open the file, press **F5** |

## Using the OpenSCAD model

At the top of `rover.scad`:

```
OPTION   = "A";     // "A" = Pi Rover, "B" = ESP32-CAM, "C" = 6-wheel rocker-bogie
EXPLODE  = 0;       // 0 = assembled, 1 = exploded view
SHOW_FOV = true;    // show the camera's field of view
```

Change a value and press F5 to redraw. Every important size is a named number (wheel size, plate size, standoff height, camera tilt...), so you can try "what if" ideas. For example, set `CAM_TILT = 0` and see how the camera's view changes.

**Want to 3D-print a part?** For example, a camera bracket or a mast mount: copy just that module into a new file, press **F6** to render, then **File → Export → STL**. The full assembly is made for *looking at*, not for printing as one piece, so OpenSCAD warns that it "may not be a valid 2-manifold". That warning is expected.

All dimensions are in millimetres and are **approximate**: parts from different sellers vary by a few mm. Measure your real parts before designing anything that must fit tightly.
