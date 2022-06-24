gcc -std=c17 vrEmuLcd.c pix80emu.c -lSDL2 -I./include -Wall -lSDL2main -lSDL2 -DVR_LCD_EMU_STATIC=1 -Ofast -o pix80emu 
