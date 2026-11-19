// =====================================================================
//  Focus Lamp: parametric CAD model (concept)
//  Open in OpenSCAD (free, https://openscad.org), press F5 to preview.
//  Units: millimetres. The lamp faces +X, Z is up.
//
//  Change POSE to see the lamp in each mode, or set the joint angles
//  yourself. Sizes are approximate: measure your real servos and
//  brackets before cutting metal or wood.
// =====================================================================

POSE = "focus";   // "rest", "focus", "read", "look", "phone", "wind", "sun", "custom"

// custom pose (used when POSE = "custom"); angles in degrees
C_YAW = 0; C_SHOULDER = 25; C_ELBOW = 80; C_WRIST = 55;

// ---------------------------------------------------------------- dimensions
BASE_D     = 150;  BASE_H = 40;   // weighted base
L1         = 160;                 // shoulder -> elbow
L2         = 140;                 // elbow -> wrist
SERVO      = [45, 25, 35];        // Feetech STS3215, approx L x W x H
BAR_W      = 18;  BAR_T = 3;      // aluminium flat bar
BAR_GAP    = 36;                  // distance between the two bars of a link
SHADE_R1   = 24;  SHADE_R2 = 42;  SHADE_L = 70;
$fn = 48;

// pose table: [yaw, shoulder, elbow, wrist] (forward lean from vertical, degrees)
function pose(name) =
    name == "rest"  ? [0,   5, 150, 20] :
    name == "focus" ? [0,  25,  80, 55] :
    name == "read"  ? [-10, 40,  70, 55] :
    name == "look"  ? [0,  10,  40, 30] :
    name == "phone" ? [18, 15,  70, 60] :
    name == "wind"  ? [0,  20, 110, 40] :
    name == "sun"   ? [0,   0,  15, 60] :
    [C_YAW, C_SHOULDER, C_ELBOW, C_WRIST];

P = pose(POSE);

// ---------------------------------------------------------------- parts
module servo_body() { color([0.1, 0.1, 0.1]) cube(SERVO, center = true); }

module link(len) {
    for (s = [-1, 1])
        color([0.72, 0.74, 0.76])
            translate([0, s * BAR_GAP / 2, len / 2]) cube([BAR_W, BAR_T, len + 20], center = true);
}

module head() {
    // shade, opening along +Z of this frame
    color([0.17, 0.17, 0.18]) translate([0, 0, 10])
        difference() {
            cylinder(h = SHADE_L, r1 = SHADE_R1, r2 = SHADE_R2);
            translate([0, 0, 2]) cylinder(h = SHADE_L, r1 = SHADE_R1 - 2, r2 = SHADE_R2 - 2);
        }
    color([1, 0.9, 0.7]) translate([0, 0, 13]) cylinder(h = 2, r = 22);              // warm LED
    color([0.23, 0.51, 0.96]) translate([0, 0, 10 + SHADE_L]) rotate_extrude() translate([SHADE_R2 - 1, 0]) circle(r = 2.4); // RGB ring
    color([0.11, 0.48, 0.29]) translate([0, SHADE_R2 + 2, 60]) cube([25, 9, 24], center = true); // camera
    color([0.71, 0.4, 0.11]) translate([9, SHADE_R2 + 7, 70]) cube([14, 2, 12], center = true);  // shutter
}

module base() {
    color([0.17, 0.17, 0.18]) cylinder(h = BASE_H, d1 = BASE_D + 6, d2 = BASE_D);
}

// ---------------------------------------------------------------- assembly
// joint angles lean "forward" toward +X, i.e. a rotation about the Y axis
base();
translate([0, 0, BASE_H + 18]) rotate([0, 0, P[0]]) {
    translate([0, 0, -17.5]) servo_body();                        // base turn servo
    color([0.17, 0.17, 0.18]) translate([0, 0, 3]) cylinder(h = 6, r = 36);
    translate([0, 0, 32]) rotate([0, P[1], 0]) {                  // shoulder
        rotate([90, 0, 0]) servo_body();
        link(L1);
        translate([0, 0, L1]) rotate([0, P[2], 0]) {              // elbow
            rotate([90, 0, 0]) servo_body();
            link(L2);
            translate([0, 0, L2]) rotate([0, P[3], 0]) {          // wrist
                rotate([90, 0, 0]) servo_body();
                translate([0, 0, 18]) head();
            }
        }
    }
}

// desk surface for scale
color([0.6, 0.47, 0.32]) translate([-200, -300, -20]) cube([900, 600, 20]);
