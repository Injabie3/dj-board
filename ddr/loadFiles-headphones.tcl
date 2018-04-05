xsdb
connect
target 3

# Change the following line to reflect your working directory.
cd C:/github/audioOutAXI/audioOutAXI.sdk/vga/images

# Images
dow -data SPLASH_SCREEN.bin          0x02100000
dow -data VGA_TEXT_ECHO.bin          0x02196004
dow -data VGA_TEXT_PITCH.bin         0x02197700
dow -data VGA_TEXT_EQUALIZER.bin     0x02198D84
dow -data VGA_TEXT_ON.bin            0x0219C708
dow -data VGA_TEXT_OFF.bin           0x0219D594
dow -data VGA_TEXT_HIGH.bin          0x0219E420
dow -data VGA_TEXT_MID.bin           0x0219FFA4
dow -data VGA_TEXT_LOW.bin           0x021A0F98
dow -data VGA_TEXT_SFX.bin           0x0‭21A225C

# Stored sounds.
dow -data anotherOne-headphones.bin  0x00504000
dow -data airhorn-headphones.bin     0x00530000