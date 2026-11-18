// =====================================================================
//  Claude-driven mini moon rover: parametric CAD model
//  Open in OpenSCAD (free, https://openscad.org), press F5 to preview.
//
//  Units: millimetres. The rover faces +X, Z is up.
//  Change OPTION below to "A", "B" or "C" to see each build.
//
//  These are the same dimensions as the interactive 3D viewer. They're
//  approximate: real parts vary by a few mm between sellers, so measure
//  yours before designing anything that must fit tightly (e.g. 3D-printed
//  mounts).
// =====================================================================

OPTION   = "A";     // "A" = Pi Rover, "B" = ESP32-CAM, "C" = 6-wheel rocker-bogie
EXPLODE  = 0;       // 0 = assembled, 1 = exploded view
SHOW_FOV = true;    // show the camera's field of view

// ---------------------------------------------------------------- key dimensions
WHEEL_D      = 65;    // wheel diameter
WHEEL_W      = 26;    // wheel width
AXLE_Z       = WHEEL_D / 2;
PLATE_L      = 260;   // chassis plate length (X)
PLATE_W      = 150;   // chassis plate width (Y)
PLATE_T      = 3;     // acrylic thickness
BOTTOM_Z     = 45;    // underside of bottom plate
STANDOFF_H   = 35;    // gap between plates
WHEELBASE    = 170;   // front axle to rear axle (4WD)
CAM_TILT     = 12;    // camera tilt, degrees down
CAM_HFOV     = 60;    // camera horizontal field of view, degrees

TOP_Z = BOTTOM_Z + PLATE_T + STANDOFF_H;   // underside of top plate

$fn = 48;

// ---------------------------------------------------------------- colours
C_ACRYLIC = [0.85, 0.93, 0.96, 0.45];
C_YELLOW  = [0.95, 0.76, 0.19];
C_TYRE    = [0.12, 0.12, 0.11];
C_HUB     = [0.94, 0.77, 0.10];
C_PCB     = [0.11, 0.48, 0.29];
C_BLUE    = [0.12, 0.35, 0.65];
C_RED     = [0.72, 0.20, 0.15];
C_BLACK   = [0.10, 0.10, 0.10];
C_BRASS   = [0.79, 0.64, 0.29];
C_BANK    = [0.23, 0.25, 0.27];
C_ARM     = [0.91, 0.89, 0.83];

function ex(v) = v * EXPLODE;   // explode offset helper

// ---------------------------------------------------------------- parts
module wheel() {
    // axis along Y
    color(C_TYRE) rotate([90, 0, 0]) cylinder(d = WHEEL_D, h = WHEEL_W, center = true);
    color(C_HUB)  rotate([90, 0, 0]) cylinder(d = 42, h = WHEEL_W + 1, center = true);
}

module tt_motor(flip = false) {
    // origin = output shaft centre; body runs 54 mm toward the rover's middle
    dir = flip ? -1 : 1;
    color(C_YELLOW) translate([-dir * 21.5, 0, 0]) cube([65, 22, 19], center = true);
    color("white") rotate([90, 0, 0]) cylinder(d = 5.5, h = 40, center = true);
}

module plate(z) {
    color(C_ACRYLIC) translate([0, 0, z + PLATE_T / 2]) cube([PLATE_L, PLATE_W, PLATE_T], center = true);
}

module standoff(x, y) {
    color(C_BRASS) translate([x, y, BOTTOM_Z + PLATE_T]) cylinder(d = 6, h = STANDOFF_H, $fn = 6);
}

module battery_pack(x, z) {
    color(C_BLACK) translate([x, 0, z + 10]) cube([71, 65, 20], center = true);
}

module breadboard(x, y, z) {
    color([0.95, 0.95, 0.92]) translate([x, y, z + 4.5]) cube([46, 35, 9], center = true);
}

module tb6612(x, y, z) {
    color(C_RED) translate([x, y, z + 0.8]) cube([25, 20, 1.6], center = true);
}

module pi_zero(x, z) {
    color(C_PCB)   translate([x, 0, z + 0.8]) cube([65, 30, 1.6], center = true);
    color(C_BLACK) translate([x, 11, z + 1.6 + 4.25]) cube([51, 5, 8.5], center = true);
}

module power_bank(x, z) {
    color(C_BANK) translate([x, 0, z + 7.5]) cube([90, 62, 15], center = true);
}

module camera_board() {
    // origin = centre of the board, lens pointing +X
    color(C_PCB)   cube([1.6, 25, 24], center = true);
    color(C_BLACK) translate([3.5, 0, 0]) cube([6, 9, 9], center = true);
    color(C_BLACK) translate([6.5, 0, 0]) rotate([0, 90, 0]) cylinder(d = 6.4, h = 5);
}

module ultrasonic() {
    // origin = centre of the board, transducers pointing +X
    color(C_BLUE) cube([1.6, 45, 20], center = true);
    for (y = [-13, 13]) color([0.75, 0.77, 0.78]) translate([0.8, y, 0]) rotate([0, 90, 0]) cylinder(d = 16, h = 12);
}

module led(x, y, z) {
    color([1, 1, 0.95]) translate([x, y, z]) rotate([0, 90, 0]) {
        cylinder(d = 5, h = 7);
        translate([0, 0, 7]) sphere(d = 5);
    }
}

module fov_cone(len = 600) {
    // translucent pyramid showing what the camera can see
    h = len * tan(CAM_HFOV / 2);
    v = len * tan(atan(tan(CAM_HFOV / 2) * 0.75));
    color([0.91, 0.46, 0.25, 0.12])
        polyhedron(
            points = [[0, 0, 0], [len, -h, -v], [len, h, -v], [len, h, v], [len, -h, v]],
            faces  = [[0, 2, 1], [0, 3, 2], [0, 4, 3], [0, 1, 4], [1, 2, 3, 4]]);
}

// ---------------------------------------------------------------- 4WD chassis (options A and B)
module chassis_4wd() {
    plate(BOTTOM_Z);
    translate([0, 0, ex(150)]) plate(TOP_Z);
    translate([0, 0, ex(75)])
        for (p = [[115, 60], [115, -60], [0, 65], [0, -65], [-115, 60], [-115, -60]]) standoff(p[0], p[1]);
    // motors under the plate, wheels outside
    for (s = [1, -1]) {
        translate([ WHEELBASE / 2, s * 61, AXLE_Z]) tt_motor(false);
        translate([-WHEELBASE / 2, s * 61, AXLE_Z]) tt_motor(true);
        translate([ WHEELBASE / 2, s * (90 + ex(70)), AXLE_Z]) wheel();
        translate([-WHEELBASE / 2, s * (90 + ex(70)), AXLE_Z]) wheel();
    }
    translate([0, 0, ex(40)]) battery_pack(-62, BOTTOM_Z + PLATE_T);
    translate([0, 0, ex(40)]) breadboard(45, 0, BOTTOM_Z + PLATE_T);
    translate([0, 0, ex(70)]) tb6612(45, 0, BOTTOM_Z + PLATE_T + 12);
    translate([0, 0, ex(170)]) power_bank(-82, TOP_Z + PLATE_T);
}

module option_a() {
    chassis_4wd();
    translate([0, 0, ex(190)]) pi_zero(15, TOP_Z + PLATE_T + 3);
    // camera on a bracket at the front edge, tilted down
    translate([ex(60), 0, ex(190)]) {
        color(C_ACRYLIC) translate([121, 0, TOP_Z + PLATE_T + 15]) cube([3, 34, 30], center = true);
        translate([126, 0, 106]) rotate([0, CAM_TILT, 0]) camera_board();
        for (y = [-34, 34]) led(125, y, 98);
    }
    // ultrasonic sensor between the plates
    translate([131 + ex(90), 0, 63 + ex(30)]) ultrasonic();
    if (SHOW_FOV && EXPLODE == 0) translate([135, 0, 104]) rotate([0, CAM_TILT, 0]) fov_cone();
}

module option_b() {
    chassis_4wd();
    translate([ex(60), 0, ex(190)]) {
        color(C_ACRYLIC) translate([118, 0, TOP_Z + PLATE_T + 9]) cube([3, 30, 18], center = true);
        color(C_BLACK)   translate([122, 0, 106]) cube([1.6, 27, 40], center = true);   // ESP32-CAM
        color(C_PCB)     translate([124.5, 0, 116]) cube([4, 8, 10], center = true);   // camera module
        color(C_BLACK)   translate([126.5, 0, 116]) rotate([0, 90, 0]) cylinder(d = 6, h = 5);
        color([1, 1, 0.95]) translate([123.5, 7, 100]) cube([2, 4, 4], center = true); // flash LED
    }
    if (SHOW_FOV && EXPLODE == 0) translate([131, 0, 116]) fov_cone();
}

// ---------------------------------------------------------------- option C: rocker-bogie
FRONT_X = 110;  MID_X = -5;  REAR_X = -115;
PIVOT   = [20, 95];     // rocker pivot (x, z) on the body wall
BOGIE   = [-60, 65];    // bogie pivot (x, z) at the rocker's rear end

module arm(p1, p2, y, w = 20, t = 5) {
    // flat bar from p1 to p2 (x, z), at side position y
    dx = p2[0] - p1[0];  dz = p2[1] - p1[1];
    len = sqrt(dx * dx + dz * dz);
    color(C_ARM) translate([p1[0], y, p1[1]]) rotate([0, -atan2(dz, dx), 0])
        translate([len / 2, 0, 0]) cube([len + w * 0.6, t, w], center = true);
}

module option_c() {
    // body: a food container with a lid
    translate([0, 0, ex(120)]) {
        color([0.91, 0.93, 0.94, 0.55]) translate([0, 0, 90]) cube([200, 120, 60], center = true);
        color([0.29, 0.56, 0.72]) translate([0, 0, 122]) cube([204, 124, 4], center = true);
    }
    for (s = [1, -1]) {
        translate([0, s * ex(40), 0]) {
            arm(PIVOT, [FRONT_X, 40], s * 68);                  // rocker, front leg
            arm(PIVOT, BOGIE, s * 68);                          // rocker, rear leg
            arm(PIVOT, [PIVOT[0], 138], s * 68, w = 12);        // differential post
        }
        translate([0, s * ex(55), 0]) {
            arm(BOGIE, [MID_X, 40], s * 75, w = 18);            // bogie legs
            arm(BOGIE, [REAR_X, 40], s * 75, w = 18);
        }
        for (x = [FRONT_X, MID_X, REAR_X]) {
            color(C_YELLOW) translate([x, s * (86 + ex(45)), 52]) cube([22, 19, 50], center = true);
            translate([x, s * (114 + ex(90)), AXLE_Z]) wheel();
        }
    }
    // differential bar across the lid
    translate([0, 0, ex(60)]) color([0.54, 0.35, 0.17]) translate([PIVOT[0], 0, 130]) cube([10, 150, 6], center = true);
    // electronics
    translate([0, 0, ex(160)]) battery_pack(-45, 62);
    translate([0, 0, ex(200)]) power_bank(40, 62);
    translate([0, 0, ex(230)]) for (y = [-25, 25]) breadboard(-50, y, 96.5);
    translate([0, 0, ex(250)]) for (y = [-25, 25]) tb6612(-50, y, 107);
    translate([0, 0, ex(160)]) pi_zero(-40, 124);
    // mast, camera, headlights
    translate([0, 0, ex(120)]) color([0.73, 0.55, 0.33]) translate([85, 0, 124]) cylinder(d = 10, h = 160);
    translate([ex(40), 0, ex(200)]) {
        translate([92, 0, 272]) rotate([0, 18, 0]) camera_board();
        for (y = [-16, 16]) led(98, y, 250);
    }
    translate([101 + ex(80), 0, 85]) ultrasonic();
    if (SHOW_FOV && EXPLODE == 0) translate([101, 0, 269]) rotate([0, 18, 0]) fov_cone();
}

// ---------------------------------------------------------------- floor + scale reference
color([0.8, 0.82, 0.79]) translate([0, 0, -0.5]) cube([800, 600, 1], center = true);
// a 12 cm drink can, for scale
color([0.72, 0.74, 0.75]) translate([-40, 230, 0]) cylinder(d = 66, h = 122);

if (OPTION == "A") option_a();
else if (OPTION == "B") option_b();
else option_c();
