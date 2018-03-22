create_clock -period 10.000 -name CLK_100 -waveform { 0.000 5.000 } [get_ports CLK_100]

set_property PACKAGE_PIN AB1 [get_ports AC_ADR0]
set_property PACKAGE_PIN Y5 [get_ports AC_ADR1]
set_property PACKAGE_PIN Y8 [get_ports AC_GPIO0]
set_property PACKAGE_PIN AA7 [get_ports AC_GPIO1]
set_property PACKAGE_PIN AA6 [get_ports AC_GPIO2]
set_property PACKAGE_PIN Y6 [get_ports AC_GPIO3]
set_property PACKAGE_PIN AB4 [get_ports AC_SCK]
set_property PACKAGE_PIN AB5 [get_ports AC_SDA]
set_property PACKAGE_PIN AB2 [get_ports AC_MCLK]
set_property PACKAGE_PIN Y9 [get_ports CLK_100]

set_property IOSTANDARD LVCMOS33 [get_ports AC_ADR0]
set_property IOSTANDARD LVCMOS33 [get_ports AC_ADR1]
set_property IOSTANDARD LVCMOS33 [get_ports AC_GPIO0]
set_property IOSTANDARD LVCMOS33 [get_ports AC_GPIO1]
set_property IOSTANDARD LVCMOS33 [get_ports AC_GPIO2]
set_property IOSTANDARD LVCMOS33 [get_ports AC_GPIO3]
set_property IOSTANDARD LVCMOS33 [get_ports AC_SCK]
set_property IOSTANDARD LVCMOS33 [get_ports AC_SDA]

set_property IOSTANDARD LVCMOS33 [get_ports AC_MCLK]

set_property IOSTANDARD LVCMOS33 [get_ports CLK_100]

# ----------------------------------------------------------------------------
# User LEDs - Bank 33
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN T22 [get_ports {LD0}];  # "LD0"
set_property PACKAGE_PIN T21 [get_ports {LD1}];  # "LD1"
set_property PACKAGE_PIN U22 [get_ports {LD2}];  # "LD2"
set_property PACKAGE_PIN U21 [get_ports {LD3}];  # "LD3"
#set_property PACKAGE_PIN V22 [get_ports {LD4}];  # "LD4"
#set_property PACKAGE_PIN W22 [get_ports {LD5}];  # "LD5"
#set_property PACKAGE_PIN U19 [get_ports {LD6}];  # "LD6"
#set_property PACKAGE_PIN U14 [get_ports {LD7}];  # "LD7"

# ----------------------------------------------------------------------------
# VGA Output - Bank 33
# ----------------------------------------------------------------------------
# Ask Maggie and Steve about the order of the pins. 
set_property PACKAGE_PIN Y21  [get_ports {VGA_B[0]}];  # "VGA-B" bit 0
set_property PACKAGE_PIN Y20  [get_ports {VGA_B[1]}];  # "VGA-B" bit 1
set_property PACKAGE_PIN AB20 [get_ports {VGA_B[2]}];  # "VGA-B" bit 2
set_property PACKAGE_PIN AB19 [get_ports {VGA_B[3]}];  # "VGA-B" bit 3

set_property PACKAGE_PIN AB22 [get_ports {VGA_G[0]}];  # "VGA-G" bit 0
set_property PACKAGE_PIN AA22 [get_ports {VGA_G[1]}];  # "VGA-G" bit 1
set_property PACKAGE_PIN AB21 [get_ports {VGA_G[2]}];  # "VGA-G" bit 2
set_property PACKAGE_PIN AA21 [get_ports {VGA_G[3]}];  # "VGA-G" bit 3

set_property PACKAGE_PIN V20  [get_ports {VGA_R[0]}];  # "VGA-R" bit 0
set_property PACKAGE_PIN U20  [get_ports {VGA_R[1]}];  # "VGA-R" bit 1
set_property PACKAGE_PIN V19  [get_ports {VGA_R[2]}];  # "VGA-R" bit 2
set_property PACKAGE_PIN V18  [get_ports {VGA_R[3]}];  # "VGA-R" bit 3

set_property PACKAGE_PIN Y19  [get_ports {VGA_VSYNC}]; # "VGA-VSYNC"
set_property PACKAGE_PIN AA19 [get_ports {VGA_HSYNC}]; # "VGA-HSYNC"

# ----------------------------------------------------------------------------
# User Push Buttons - Bank 34
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN P16 [get_ports {btns_5bits[0]}];  # "BTNC"
set_property PACKAGE_PIN R16 [get_ports {btns_5bits[1]}];  # "BTND"
set_property PACKAGE_PIN N15 [get_ports {btns_5bits[2]}];  # "BTNL"
set_property PACKAGE_PIN R18 [get_ports {btns_5bits[3]}];  # "BTNR"
set_property PACKAGE_PIN T18 [get_ports {btns_5bits[4]}];  # "BTNU"

# ----------------------------------------------------------------------------
# User DIP Switches - Bank 35
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN F22 [get_ports {sws_8bits[0]}];  # "SW0"
set_property PACKAGE_PIN G22 [get_ports {sws_8bits[1]}];  # "SW1"
set_property PACKAGE_PIN H22 [get_ports {sws_8bits[2]}];  # "SW2"
set_property PACKAGE_PIN F21 [get_ports {sws_8bits[3]}];  # "SW3"
set_property PACKAGE_PIN H19 [get_ports {sws_8bits[4]}];  # "SW4"
set_property PACKAGE_PIN H18 [get_ports {sws_8bits[5]}];  # "SW5"
set_property PACKAGE_PIN H17 [get_ports {sws_8bits[6]}];  # "SW6"
set_property PACKAGE_PIN M15 [get_ports {sws_8bits[7]}];  # "SW7"

set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 33]];
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 34]];
set_property IOSTANDARD LVCMOS18 [get_ports -of_objects [get_iobanks 35]];