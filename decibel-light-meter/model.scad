// Decibel Light Meter — parametric tower + weighted base
// Target printer: Bambu D2X. Default segment height (220mm) is a safe bet for
// most Bambu beds — bump SEGMENT_H up if your D2X's Z height is larger.
//
// Render one part at a time: set PART below, then F6 -> export STL.
// PART = "base" | "segment" | "cap" | "diffuser"

PART = "base";

// ---- Global tower parameters ----
OUTER_D     = 90;     // outer diameter of the tower, mm
WALL        = 2.4;    // wall thickness, mm (6 perimeters at 0.4mm line width)
TOTAL_H     = 1000;   // total tower height target, mm
SEGMENT_H   = 220;    // printable segment height, mm (fits most beds w/ margin)
WINDOW_W    = 50;     // width of the LED diffusion window per segment
WINDOW_INSET= 4;      // how far the window recesses into the wall, for diffuser to sit in
JOINT_D     = 4;       // depth of the stacking lap joint
JOINT_CLEAR = 0.15;    // clearance for the lap joint fit
ROD_D       = 8.5;     // center hole for M8 threaded rod (structural spine)
$fn = 96;

// ---- Base parameters ----
BASE_D        = 220;
BASE_H        = 80;
BASE_WALL     = 3.0;
WEIGHT_CAVITY_H = 55;   // fill with sand/lead shot/epoxy for ballast
ELEC_BAY_D    = 70;     // electronics bay diameter (mic, ESP32, battery/USB)
ELEC_BAY_H    = 20;
MIC_HOLE_D    = 6;      // sound port to mic capsule
USBC_SLOT_W   = 10;
USBC_SLOT_H   = 6;

module tower_outer(h) {
    cylinder(d = OUTER_D, h = h);
}

module tower_hollow(h) {
    translate([0,0,-1])
        cylinder(d = OUTER_D - 2*WALL, h = h + 2);
}

module led_window(h) {
    // vertical slot facing +Y, recessed for a diffuser strip to clip into
    translate([0, OUTER_D/2 - WINDOW_INSET, h/2])
        rotate([90,0,0])
        linear_extrude(height = WINDOW_INSET + WALL + 1)
            offset(r=2) square([WINDOW_W - 4, h - 30], center = true);
}

module rod_channel(h) {
    translate([0,0,-1]) cylinder(d = ROD_D, h = h + 2);
}

module lap_joint_male(h) {
    // protrusion on top of a segment that plugs into the next segment's socket
    translate([0,0,h])
        cylinder(d = OUTER_D - 2*WALL - JOINT_CLEAR, h = JOINT_D);
}

module lap_joint_female() {
    translate([0,0,-1])
        cylinder(d = OUTER_D - 2*WALL + JOINT_CLEAR, h = JOINT_D + 1);
}

module segment(h = SEGMENT_H, bottom_socket = true, top_plug = true) {
    difference() {
        union() {
            cylinder(d = OUTER_D, h = h);
            if (top_plug) lap_joint_male(h);
        }
        tower_hollow(h);
        led_window(h);
        rod_channel(h + JOINT_D);
        if (bottom_socket) lap_joint_female();
    }
}

module cap() {
    // top cap, frosted/solid — print in white/translucent PETG for diffusion
    difference() {
        union() {
            cylinder(d = OUTER_D, h = 8);
            cylinder(d = OUTER_D - 6, h = 18);
        }
        translate([0,0,-1]) cylinder(d = ROD_D + 1, h = 10); // rod nut pocket
        lap_joint_female();
    }
}

module diffuser_strip(h = SEGMENT_H - 30) {
    // thin frosted insert that clips into the window cutout; print separately
    // in white/translucent PETG, 0.6mm walls, no infill.
    difference() {
        cube([WINDOW_W - 4, 1.2, h], center = true);
    }
}

module base() {
    difference() {
        union() {
            cylinder(d = BASE_D, h = BASE_H);
        }
        // ballast cavity (fill with sand/shot after printing, then cap)
        translate([0,0,BASE_WALL])
            cylinder(d = BASE_D - 2*BASE_WALL, h = WEIGHT_CAVITY_H);
        // electronics bay, raised above ballast
        translate([0,0,WEIGHT_CAVITY_H + BASE_WALL])
            cylinder(d = ELEC_BAY_D, h = ELEC_BAY_H + 1);
        // mic sound port straight up to where the tower base sits
        translate([0,0,BASE_H - 5]) cylinder(d = MIC_HOLE_D, h = 10);
        // USB-C access slot on the side, at electronics bay height
        translate([BASE_D/2 - BASE_WALL - 1, 0, WEIGHT_CAVITY_H + BASE_WALL + ELEC_BAY_H/2])
            rotate([0,90,0])
            linear_extrude(height = BASE_WALL + 2)
                offset(r=1.5) square([USBC_SLOT_H, USBC_SLOT_W], center = true);
        // rod anchor hole at center, threads into a heat-set insert
        translate([0,0,BASE_H - 12]) cylinder(d = ROD_D, h = 14);
        // socket for first tower segment to seat into
        translate([0,0,BASE_H - JOINT_D])
            cylinder(d = OUTER_D - 2*WALL + JOINT_CLEAR, h = JOINT_D + 1);
    }
}

// ---- render selection ----
if (PART == "base") base();
else if (PART == "segment") segment();
else if (PART == "cap") cap();
else if (PART == "diffuser") diffuser_strip();
