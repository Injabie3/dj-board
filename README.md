# ENSC 452 - Group 15 - The Ultimate DJ Board

## Overview
This is our group project for ENSC 452 - Advanced Digital System Design.  We decided to do audio processing in the form of a DJ board.

## Requirements
* Zedboard
* Vivado 2017.3
* Vivado 2017.3 Tcl Shell
* Xilinx SDK

## Usage

### Vivado
1. Open Vivado.
2. Run `build.tcl`.  This will generate the block diagram.
3. Click `Generate Bitstream`.  This will take a while, so grab yourself a coffee and get comfortable!
4. Export the hardware.
5. Launch the SDK with default settings.

### Xilinx SDK
1. Import projects from `./sdk`.
2. Create a board support package (BSP) with the name `audioTest_bsp` and the following setting:  
   Uncheck **Use default location**.  
   Location: **(browse to `./sdk` folder)**  
   CPU: **ps7_cortexa9_0**.
3. Build the project.
4. Program the FPGA with the bitstream that you exported.

### Set Up Debug Configurations
Make a debug configuration with the following name:

1. **Run Me First!**  
   In the `Target Setup` tab:  
   Debug Type: **Standalone Application Debug**.  
   Uncheck **Program FPGA**.  
   Check **Run ps7\_post\_config**.  
   In the `Application` tab:  
   Core 0: **audioTest/audioTest.elf**  
   Core 1: **(none)**

### Run Project
1. Ensure that **Skip All Breakpoints** is deselected.
2. Debug `Run Me First!`, and click **Continue** when loaded.

## Acknowledgements
- AXI-Lite Slave from [Laxer3a](https://github.com/Laxer3a/ZedBoardAudio)