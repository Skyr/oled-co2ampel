wall_thickness = 3;
screw_size = 2; // M2 screw
screwhole_size = 1.7;  // For M2 screws
screwblock_diam = 1.5*wall_thickness + screwhole_size;
board_x = 82;
board_y = 51;
board_spacing = 3;
screw_height = 3;

module screw_opening(height) {
    translate([0,0,height/2]) 
        cylinder(height, d=screw_size, center=true, $fn=30);
}

module screw_hole(height) {
    translate([0,0,height/2]) 
        cylinder(height, d=screwhole_size, center=true, $fn=30);
}

module screw_block(height, diam=screwblock_diam) {
    translate([0,0,height/2]) cylinder(height, d=diam, center=true, $fn=30);
}

module screw_mount(height, diam=screwblock_diam) {
    difference() {
        screw_block(height, diam);
        screw_hole(height);
    }
}

module ventilation_slit(height) {
    width=2;
    translate([0,wall_thickness,0])
    rotate([90,0,0])
    linear_extrude(height=wall_thickness)
        polygon(points=[ [0,0], [-width/2, 5], [-width/2,height-5], [0,height], [width/2,height-5],[width/2,5] ]);
}

module box() {
    board_screw_offset = 5.25;
    electronics_height = 30;

    module outer_box() {
        difference() {
            cube([2*wall_thickness + 2*board_spacing + board_x,
                2*wall_thickness + 2*board_spacing + board_y,
                wall_thickness + screw_height + electronics_height ]);
            translate([wall_thickness,wall_thickness,wall_thickness])
                cube([2*board_spacing + board_x,
                    2*board_spacing + board_y,
                    screw_height + electronics_height ]);
        }
    }

    // Outer box with screw holes
    difference() {
        union() {
            outer_box();
            for (i=[0:1]) {
                for (j=[0:1]) {
                    translate([screwblock_diam/2 + i*(2*wall_thickness + 2*board_spacing + board_x-screwblock_diam), 
                        screwblock_diam/2 + j*(2*wall_thickness + 2*board_spacing + board_y-screwblock_diam),
                        wall_thickness])
                            screw_block(screw_height + electronics_height);
                }
            }
        }
        // Holes for screws
        for (i=[0:1]) {
            for (j=[0:1]) {
                translate([screwblock_diam/2 + i*(2*wall_thickness + 2*board_spacing + board_x-screwblock_diam), 
                    screwblock_diam/2 + j*(2*wall_thickness + 2*board_spacing + board_y-screwblock_diam),
                    wall_thickness + electronics_height ])
                        screw_hole(screw_height);
            }
        }
        // USB opening
        usb_width = 14;
        usb_height = 10;
        translate([wall_thickness+board_spacing+49-(usb_width/2),0,wall_thickness+screw_height+8-(usb_height/2)]) cube([usb_width,wall_thickness,usb_height]);
        // Ventilation slits
        for (i=[0:5]) {
            translate([wall_thickness+board_spacing+2+i*6,0,wall_thickness+screw_height])
                ventilation_slit(electronics_height-screw_height);
            translate([wall_thickness+board_spacing+2+i*6,wall_thickness+2*board_spacing+board_y,wall_thickness+screw_height])
                ventilation_slit(electronics_height-screw_height);
        }
        for (i=[0:7]) {
            translate([wall_thickness,wall_thickness+board_spacing+board_y/2-1+(i-3.5)*6,wall_thickness+screw_height])
            rotate([0,0,90])
                ventilation_slit(electronics_height-screw_height);
        }
    }
    
    translate([0,0,wall_thickness]) {
        for (i=[0:1]) {
            for (j=[0:1]) {
                translate([wall_thickness+board_spacing+board_screw_offset + i*(board_x-2*board_screw_offset),
                    wall_thickness+board_spacing+board_screw_offset + j*(board_y-2*board_screw_offset),
                    0]) screw_mount(screw_height);
            }
        }
    }
    
    // For reference: Board
    //translate([wall_thickness+board_spacing, wall_thickness+board_spacing, wall_thickness+screw_height]) 
    //    color("red")    
    //    cube([board_x, board_y, 2]);
}


module lid() {
    thickness = wall_thickness;
    display_x_ofs = 32;  // Offset from top of board
    display_x_out_of_center = 1;  // Display not exactly in middle of board...
    display_x = 14;
    display_y = 25;
    screw_x_ofs = 4;
    screw_y_ofs = 1.5;
    
    difference() {
        cube([2*wall_thickness + 2*board_spacing + board_x,
            2*wall_thickness + 2*board_spacing + board_y,
            thickness]);
        // Screw holes for mounting the lid
        for (i=[0:1]) {
            for (j=[0:1]) {
                translate([screwblock_diam/2 + i*(2*wall_thickness + 2*board_spacing + board_x-screwblock_diam), 
                    screwblock_diam/2 + j*(2*wall_thickness + 2*board_spacing + board_y-screwblock_diam),
                    0 ])
                        screw_opening(thickness);
            }
        }
        // Display opening
        translate([wall_thickness + board_spacing + display_x_ofs + display_x_out_of_center,(2*wall_thickness + 2*board_spacing + board_y - display_y)/2,0])
            cube([display_x,display_y,thickness]);
    }
    // Screw mounts for display
    translate([wall_thickness + board_spacing + display_x_ofs,(2*wall_thickness + 2*board_spacing + board_y - display_y)/2,-screw_height]) {
        translate([-screw_x_ofs,screw_y_ofs,0]) screw_mount(screw_height, diam = screwhole_size + 2.5);
        translate([display_x+screw_x_ofs,screw_y_ofs,0]) screw_mount(screw_height, screwhole_size + 2.5);
        translate([-screw_x_ofs,display_y-screw_y_ofs,0]) screw_mount(screw_height, screwhole_size + 2.5);
        translate([display_x+screw_x_ofs,display_y-screw_y_ofs,0]) screw_mount(screw_height, screwhole_size + 2.5);
    }
}


translate([0,0,-30 - 20])
    box();
// rotate([0,180,0])
    lid();
