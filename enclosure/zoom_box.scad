// =====================================================================
//  Zoom Control Box  -  parametric enclosure (OpenSCAD 2021.01+)
//  XIAO SAMD21 + Grove Shield for XIAO (full size) + 4x Grove Red LED Button
//
//  Parts (set `part`, or use the CLI commands at the bottom of this header):
//    "assembly"  exploded preview with ghost electronics
//    "base"      print upright, as oriented
//    "lid"       print face-down, as oriented (already flipped)
//    "labels"    label inlay bodies, same coordinates as "lid" (multi-material)
//    "guard"     MUTE ring guard, glue/peg onto lid after printing
//
//  CLI export:
//    openscad -o base.stl   -D 'part="base"'   zoom_box.scad
//    openscad -o lid.stl    -D 'part="lid"'    zoom_box.scad
//    openscad -o labels.stl -D 'part="labels"' zoom_box.scad
//    openscad -o guard.stl  -D 'part="guard"'  zoom_box.scad
//
//  Coordinates: X = left->right, Y = front->back, Z = up.
//  Values marked (MEASURE) are datasheet/photo estimates. Verify with calipers.
// =====================================================================

part = "assembly"; // [assembly, base, lid, labels, guard]

/* ---------------- Print / global ---------------- */
$fn        = 64;
wall       = 2.0;    // side walls
floor_t    = 2.0;    // base floor
lid_t      = 3.0;    // lid plate thickness
tol        = 0.2;    // clearance on mating faces
slope      = 12;     // front-to-back tilt of the top face, degrees (10-15)

/* ---------------- Grove Red LED Button module ---------------- */
// Module-local frame: PCB corner at origin, X across (20), Y along (40),
// Y=0 is the FRONT end (opposite the Grove connector), Y=40 the connector end.
btn_pcb_w         = 20;
btn_pcb_l         = 40;
btn_pcb_t         = 1.6;
btn_total_h       = 16.0;  // PCB bottom to top of red cap (MEASURE)
btn_cap_d         = 10.0;  // red cap diameter (MEASURE)
btn_cap_from_conn = 24.0;  // cap centre from connector end of PCB (MEASURE)
btn_cap_off_x     = 0.0;   // cap offset from PCB centreline (MEASURE)
btn_holes = [[2.5, 2.5], [17.5, 2.5], [2.5, 37.5], [17.5, 37.5]]; // M2 (MEASURE)
btn_housing_h     = 8.0;   // top of white switch housing above PCB top (MEASURE)
btn_housing_w     = 14.0;  // housing square size, for underside relief (MEASURE)
btn_under_clear   = 2.5;   // solder joints / pins below PCB

cap_protrude  = 2.0;       // cap height above lid top
hole_clear    = 0.4;       // added to cap diameter for the lid hole
standoff_d    = 4.6;
m2_pilot_d    = 1.7;       // M2 self-tapping into PLA/PETG (use 1.6 for PETG if loose)

/* ---------------- Layout ---------------- */
labels          = ["MUTE", "VIDEO", "HAND", "FULL"];
label_size      = [5, 5, 5, 5];   // cap height of text, mm
label_depth     = 0.6;
label_gap       = 2.0;            // gap between hole/guard and label
label_font      = "Liberation Sans:style=Bold";
label_mode      = "inlay";        // "inlay" (pockets + labels part) or "deboss"

side_margin     = 8;              // outer edge to first/last module
btn_gap         = 10;             // gap between modules
btn_row_y       = 10;             // lid-local Y of module front ends
full_offset_y   = 5;              // FULL pushed back (de-emphasised)
btn_dy          = [0, 0, 0, full_offset_y];
cable_clear     = 8;              // behind modules for Grove cable bend

guard_od        = 26;             // MUTE ring guard
guard_id        = 17;
guard_extra     = 1.5;            // ring top above cap top
guard_peg_d     = 2.4;
guard_peg_h     = 2.0;

/* ---------------- Grove Shield for XIAO + XIAO SAMD21 ---------------- */
// Shield-local frame: X along 58 mm, X=58 is the XIAO / USB-C end.
shield_l           = 58;
shield_w           = 25;
shield_t           = 1.6;
shield_standoff_h  = 3.0;
shield_top_clear   = 15;     // XIAO + Grove plugs + cables
shield_end_gap     = 0.5;    // USB end to inside of right wall
shield_holes = [[3.5, 2.5], [38.5, 2.5], [3.5, 22.5], [38.5, 22.5]]; // M2 (MEASURE)
usb_w              = 12;     // USB-C plug overmold width (MEASURE)
usb_h              = 7;      // USB-C plug overmold height (MEASURE)
usb_clear          = 0.75;   // per side
usb_z_above_shield = 5.0;    // receptacle centre above shield top (MEASURE)
usb_y_off          = 0.0;    // receptacle centre offset from shield centreline
rst_pos            = [shield_l - 6, shield_w / 2 + 3]; // RST pads, shield frame (MEASURE)
rst_win            = [8, 6]; // reset/LED window size (X, Y)

/* ---------------- Fasteners ---------------- */
// Default: M3 heat-set inserts (e.g. M3 x 5.7 short insert, 4.0 mm hole)
// + M3 x 8 countersunk screws. For M2 inserts: insert_d 3.2, screw_d 2.4, cs_d 4.4.
insert_d    = 4.0;
insert_h    = 5.7;
boss_d      = 9.0;
screw_d     = 3.4;   // clearance through lid
cs_d        = 6.4;   // countersink top diameter (90 deg)

/* ---------------- Feet ---------------- */
foot_d      = 8.4;   // 8 mm bumpon + clearance
foot_depth  = 1.0;
foot_inset  = 10;

/* =====================================================================
   Derived geometry (normally no edits below)
   ===================================================================== */
lip_h = 3.0;
lip_w = 1.6;

standoff_h = btn_total_h - btn_pcb_t - lid_t - cap_protrude;
hang       = standoff_h + btn_pcb_t + btn_under_clear;     // below lid underside
relief_d   = max(0, btn_housing_h - standoff_h + 0.5);

slot0 = max(guard_od, btn_pcb_w);
bx0 = side_margin + slot0 / 2;
bx1 = bx0 + slot0 / 2 + btn_gap + btn_pcb_w / 2;
bx2 = bx1 + btn_pcb_w + btn_gap;
bx3 = bx2 + btn_pcb_w + btn_gap;
btn_x = [bx0, bx1, bx2, bx3];                 // module centrelines
W = bx3 + btn_pcb_w / 2 + side_margin;

function cap_xy(i) = [btn_x[i] + btn_cap_off_x,
                      btn_row_y + btn_dy[i] + (btn_pcb_l - btn_cap_from_conn)];

// lid-local (y, z) -> world y
function wy(y, z) = y * cos(slope) - z * sin(slope);

max_dy        = max(btn_dy);
mod_back_w    = wy(btn_row_y + max_dy + btn_pcb_l, -hang);
shield_y0     = mod_back_w + cable_clear;
D             = shield_y0 + shield_w + 0.5 + boss_d + wall - 1;

hf_need = floor_t + 1.5 + hang * cos(slope) - btn_row_y * sin(slope);
hf      = max(hf_need, floor_t + insert_h + 4);   // front height
hb      = hf + D * tan(slope);                     // back height
L       = D / cos(slope);                          // lid length along slope

shield_x0 = W - wall - shield_end_gap - shield_l;  // world
shield_z0 = floor_t + shield_standoff_h;           // shield bottom

// fastener points on the slope plane (world X, world Y)
ins_shift = (insert_h + 1) / 2 * sin(slope);
screw_pts = [[wall + boss_d / 2 - 1,     wall + boss_d / 2 - 1 - ins_shift],
             [W - wall - boss_d / 2 + 1, wall + boss_d / 2 - 1 - ins_shift],
             [wall + boss_d / 2 - 1,     D - wall - boss_d / 2 + 1 - ins_shift],
             [W - wall - boss_d / 2 + 1, D - wall - boss_d / 2 + 1 - ins_shift]];

// sanity checks
assert(relief_d < lid_t - 1.0, "Switch housing too tall: increase btn_total_h/standoff or lid_t");
assert(standoff_h > 2, "Standoffs too short: check btn_total_h / cap_protrude");
assert(shield_z0 + shield_t + shield_top_clear < hf + shield_y0 * tan(slope),
       "Not enough height over the shield");

echo(str("BOX  W=", W, "  D=", D, "  front H=", hf, "  back H=", hb));
echo(str("Button standoff height=", standoff_h, "  underside relief=", relief_d));

/* =====================================================================
   Helpers
   ===================================================================== */
module lidT() { translate([0, 0, hf]) rotate([slope, 0, 0]) children(); }

module wedge() {   // outer solid: Y/Z profile extruded along X
    rotate([90, 0, 90]) linear_extrude(W)
        polygon([[0, 0], [D, 0], [D, hb], [0, hf]]);
}

module label_shapes(extra = 0) {
    for (i = [0 : 3]) {
        c  = cap_xy(i);
        rc = (i == 0) ? guard_od / 2 : (btn_cap_d + hole_clear) / 2;
        translate([c[0], c[1] - rc - label_gap - label_size[i] / 2, lid_t - label_depth])
            linear_extrude(label_depth + extra)
                text(labels[i], size = label_size[i], font = label_font,
                     halign = "center", valign = "center");
    }
}

/* =====================================================================
   BASE
   ===================================================================== */
module base() {
    difference() {
        intersection() {
            wedge();
            union() {
                difference() {
                    cube([W, D, hb + 1]);
                    translate([wall, wall, floor_t]) cube([W - 2 * wall, D - 2 * wall, hb + 10]);
                }
                for (p = screw_pts)                        // boss columns
                    translate([p[0], p[1] + ins_shift, 0]) cylinder(d = boss_d, h = hb + 1);
                for (h = shield_holes)                     // shield standoffs
                    translate([shield_x0 + h[0], shield_y0 + h[1], 0])
                        cylinder(d = 5, h = shield_z0);
            }
        }
        // heat-set insert holes, along the lid normal
        for (p = screw_pts)
            lidT() translate([p[0], p[1] / cos(slope), -(insert_h + 1)])
                cylinder(d = insert_d, h = insert_h + 3);
        // shield pilot holes
        for (h = shield_holes)
            translate([shield_x0 + h[0], shield_y0 + h[1], floor_t])
                cylinder(d = m2_pilot_d, h = shield_z0);
        // USB-C cutout, right wall
        uy = shield_y0 + shield_w / 2 + usb_y_off;
        uz = shield_z0 + shield_t + usb_z_above_shield;
        cw = usb_w + 2 * usb_clear;
        ch = usb_h + 2 * usb_clear;
        translate([W - wall - 1, uy, uz]) rotate([0, 90, 0])
            hull() for (sy = [-1, 1], sz = [-1, 1])
                translate([sz * (ch / 2 - 1.5), sy * (cw / 2 - 1.5), 0])
                    cylinder(r = 1.5, h = wall + 2);
        // rubber-foot recesses
        for (fx = [foot_inset, W - foot_inset], fy = [foot_inset, D - foot_inset])
            translate([fx, fy, -0.01]) cylinder(d = foot_d, h = foot_depth + 0.01);
    }
}

/* =====================================================================
   LID  (built in lid-local frame, placed in world, trimmed to footprint)
   ===================================================================== */
module lid_local() {
    difference() {
        union() {
            cube([W, L + 5, lid_t]);
            // locating lip, inset for the tilt so it clears the walls
            ly0 = (wall + tol) / cos(slope);
            ly1 = (D - wall - tol - lip_h * sin(slope)) / cos(slope);
            difference() {
                translate([wall + tol, ly0, -lip_h])
                    cube([W - 2 * (wall + tol), ly1 - ly0, lip_h + 0.01]);
                translate([wall + tol + lip_w, ly0 + lip_w, -lip_h - 1])
                    cube([W - 2 * (wall + tol + lip_w), ly1 - ly0 - 2 * lip_w, lip_h + 2]);
                for (p = screw_pts)
                    translate([p[0], p[1] / cos(slope), -lip_h - 1])
                        cylinder(d = boss_d + 3, h = lip_h + 1.01);
            }
            // button standoffs
            for (i = [0 : 3], h = btn_holes)
                translate([btn_x[i] - btn_pcb_w / 2 + h[0],
                           btn_row_y + btn_dy[i] + h[1], -standoff_h])
                    cylinder(d = standoff_d, h = standoff_h + 0.01);
        }
        // cap holes + housing relief
        for (i = [0 : 3]) {
            c = cap_xy(i);
            translate([c[0], c[1], -1]) cylinder(d = btn_cap_d + hole_clear, h = lid_t + 2);
            if (relief_d > 0)
                translate([c[0] - (btn_housing_w + 0.8) / 2, c[1] - (btn_housing_w + 0.8) / 2, -0.01])
                    cube([btn_housing_w + 0.8, btn_housing_w + 0.8, relief_d]);
        }
        // standoff pilot holes
        for (i = [0 : 3], h = btn_holes)
            translate([btn_x[i] - btn_pcb_w / 2 + h[0],
                       btn_row_y + btn_dy[i] + h[1], -standoff_h - 0.1])
                cylinder(d = m2_pilot_d, h = standoff_h + 1);
        // label pockets
        label_shapes(0.1);
        // countersunk screw holes
        for (p = screw_pts)
            translate([p[0], p[1] / cos(slope), 0]) {
                translate([0, 0, -lip_h - 1]) cylinder(d = screw_d, h = lid_t + lip_h + 2);
                translate([0, 0, lid_t - (cs_d - screw_d) / 2])
                    cylinder(d1 = screw_d, d2 = cs_d, h = (cs_d - screw_d) / 2 + 0.01);
                translate([0, 0, lid_t]) cylinder(d = cs_d, h = 1);
            }
        // guard peg holes
        c0 = cap_xy(0);
        for (s = [-1, 1])
            translate([c0[0] + s * (guard_od + guard_id) / 4, c0[1], lid_t - guard_peg_h - 0.2])
                cylinder(d = guard_peg_d + 0.3, h = guard_peg_h + 1);
    }
}

module lid_world() {
    intersection() {
        translate([0, 0, -50]) cube([W, D, hb + 100]);
        difference() {
            lidT() lid_local();
            // reset / status-LED window above the XIAO (vertical)
            translate([shield_x0 + rst_pos[0] - rst_win[0] / 2,
                       shield_y0 + rst_pos[1] - rst_win[1] / 2, 0])
                cube([rst_win[0], rst_win[1], hb + 20]);
        }
    }
}

module labels_world() { lidT() label_shapes(0); }

/* =====================================================================
   MUTE ring guard (separate part, pegs into lid)
   ===================================================================== */
guard_h = cap_protrude + guard_extra;
module guard_local() {   // z=0 sits on lid top
    difference() {
        union() {
            cylinder(d = guard_od, h = guard_h);
            for (s = [-1, 1])
                translate([s * (guard_od + guard_id) / 4, 0, -guard_peg_h])
                    cylinder(d = guard_peg_d, h = guard_peg_h + 0.01);
        }
        translate([0, 0, -1]) cylinder(d = guard_id, h = guard_h + 2);
        // soft inner chamfer for the fingertip
        translate([0, 0, guard_h - 1]) cylinder(d1 = guard_id, d2 = guard_id + 2.4, h = 1.01);
    }
}

/* =====================================================================
   Ghost electronics (preview only)
   ===================================================================== */
module ghosts() {
    for (i = [0 : 3]) lidT() {
        translate([btn_x[i] - btn_pcb_w / 2, btn_row_y + btn_dy[i], -standoff_h - btn_pcb_t])
            color("royalblue") cube([btn_pcb_w, btn_pcb_l, btn_pcb_t]);
        c = cap_xy(i);
        translate([c[0] - btn_housing_w / 2, c[1] - btn_housing_w / 2, -standoff_h])
            color("white") cube([btn_housing_w, btn_housing_w, btn_housing_h]);
        translate([c[0], c[1], -standoff_h])
            color("red") cylinder(d = btn_cap_d, h = btn_total_h - btn_pcb_t);
    }
    translate([shield_x0, shield_y0, shield_z0]) {
        color("black") cube([shield_l, shield_w, shield_t]);
        translate([shield_l - 21, (shield_w - 17.5) / 2, shield_t + usb_z_above_shield - 3])
            color("silver") cube([21, 17.5, 3]);
    }
}

/* =====================================================================
   Part dispatch
   ===================================================================== */
module to_print_lid() {   // world -> lid flat, top face on bed
    translate([0, 0, lid_t]) rotate([180, 0, 0])
        rotate([-slope, 0, 0]) translate([0, 0, -hf]) children();
}

if (part == "base") base();
else if (part == "lid") to_print_lid() lid_world();
else if (part == "labels") { if (label_mode == "inlay") to_print_lid() labels_world(); }
else if (part == "guard") translate([0, 0, guard_h]) rotate([180, 0, 0]) guard_local();
else {  // assembly, exploded 25 mm
    color("dimgray") base();
    translate([0, 0, 25]) {
        color("gainsboro") lid_world();
        if (label_mode == "inlay") color("lime") labels_world();
        c0 = cap_xy(0);
        lidT() translate([c0[0], c0[1], lid_t + 6]) color("orangered") guard_local();
    }
    %ghosts();
}
