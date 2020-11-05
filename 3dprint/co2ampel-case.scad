wall_thickness = 3;
screwhole_size = 1.5;  // For M2 screws
screwblock_diam = 1.5*wall_thickness + screwhole_size;

module screw_hole(height) {
    translate([0,0,height/2]) 
        cylinder(height, d=screwhole_size, center=true, $fn=30);
}

module screw_block(height) {
    translate([0,0,height/2]) cylinder(height, d=screwblock_diam, center=true, $fn=30);
}

module screw_mount(height) {
    difference() {
        screw_block(height);
        screw_hole(height);
    }
}

module box() {
    board_x = 82;
    board_y = 51;
    board_spacing = 3;
    board_screw_offset = 5.5;
    screw_height = 4;
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
        for (i=[0:1]) {
            for (j=[0:1]) {
                translate([screwblock_diam/2 + i*(2*wall_thickness + 2*board_spacing + board_x-screwblock_diam), 
                    screwblock_diam/2 + j*(2*wall_thickness + 2*board_spacing + board_y-screwblock_diam),
                    wall_thickness + electronics_height ])
                        screw_hole(screw_height);
            }
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


box();
