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
# User Push Buttons - Bank 34
# ---------------------------------------------------------------------------- 
set_property PACKAGE_PIN P16 [get_ports {btns_5bits[0]}];  # "BTNC"
set_property PACKAGE_PIN R16 [get_ports {btns_5bits[1]}];  # "BTND"
set_property PACKAGE_PIN N15 [get_ports {btns_5bits[2]}];  # "BTNL"
set_property PACKAGE_PIN R18 [get_ports {btns_5bits[3]}];  # "BTNR"
set_property PACKAGE_PIN T18 [get_ports {btns_5bits[4]}];  # "BTNU"
set_property IOSTANDARD LVCMOS25 [get_ports -of_objects [get_iobanks 34]];