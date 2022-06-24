/*
 *  ___        ___  __   ___
 * | . \<>__  < . >|  | | __| _ _  _  _
 * |  _/||\ \// . \| \| | _| / | \| || |
 * |_|  ||/\_\\___/`__' |___|| | |\____|
 *
 * An Emulator written in C, made to emulate 
 * the Z80-based Pix80 Computer.
 * It should, best case, run all software
 * flawlessly and with high accuracy.
 *
 */


#include "./include/vrEmuLcd.h"
#define CHIPS_IMPL
#include "./include/z80.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <SDL2/SDL.h>

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)

#define LCD_WIDTH 20
#define LCD_HEIGHT 4

// Initialize a few things
VrEmuLcd *lcd;
SDL_Renderer *renderer;

bool running = true;
SDL_Event event;
int delayTime;

// Window dimensions
static const int width = 119;
static const int height = 35;

// Get the current I/O Device number
uint16_t getDevice(uint64_t pins) {
    const uint16_t addr = Z80_GET_ADDR(pins) & 0b111;
	return addr;
}

// Update and Redraw LCD
int refreshLCD() {
	// generates a snapshot of the pixels state
	vrEmuLcdUpdatePixels(lcd);
	
	// Scale Renderer
	// TODO: Find a nice way of doing this that doesn't need to be run every frame. Just seems wasteful, even if it's minor.
	int w;
	int h;
	SDL_GetRendererOutputSize(renderer, &w, &h);
	int windowWidth = w;
	int windowHeight = h;
	SDL_RenderSetScale(renderer, (windowWidth / width), (windowHeight / height));
	
	// Clear screen
	// The rendercolor shouldn't matter for this
	//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	// Plus the screen doesn't need to be cleared, since it's drawn over anyways
	SDL_RenderClear(renderer); 
	
	// Loop through LCD Pixels
	for (int y = 0; y < vrEmuLcdNumPixelsY(lcd); ++y) {
	  for (int x = 0; x < vrEmuLcdNumPixelsX(lcd); ++x) {
	   // values returned are:  -1 = no pixel (character borders), 0 = pixel off, 1 = pixel on
		char pixel = vrEmuLcdPixelState(lcd, x, y);
		switch (pixel) {
			case -1:	// Character Borders
				SDL_SetRenderDrawColor(renderer, 50, 72, 253, 255);
				//OLD SDL_SetRenderDrawColor(renderer, 31, 139, 255, 255);
				SDL_RenderDrawPoint(renderer,x,y);
				break;
			case 0:		// Pixel Off
				SDL_SetRenderDrawColor(renderer, 50, 60, 254, 255);
				// OLD SDL_SetRenderDrawColor(renderer, 61, 171, 255, 255);
				SDL_RenderDrawPoint(renderer,x,y);
				break;
			case 1:		// Pixel On
				SDL_SetRenderDrawColor(renderer, 240, 252, 253, 255);
				// OLD SDL_SetRenderDrawColor(renderer, 245, 253, 255, 255);
				SDL_RenderDrawPoint(renderer,x,y);
				break;
		}
	  }
	}
	
	// Show what was drawn
	SDL_RenderPresent(renderer);
	return 0;
}

int main(int argc, char **argv) {
	// Get the delayTime for slowmode in milliseconds
	delayTime = atoi(argv[2]);
	
	// ---------------------- Rendering ----------------------
    // Initialize SDL
    CHECK_ERROR(SDL_Init(SDL_INIT_VIDEO) != 0, SDL_GetError());

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("Pix80Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    CHECK_ERROR(window == NULL, SDL_GetError());

    // Create a renderer (accelerated and in sync with the display refresh rate)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    
    CHECK_ERROR(renderer == NULL, SDL_GetError());

	// Scale Window an arbitrary amount and center it
	SDL_SetWindowSize(window,width*7,height*7);
	SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
	
    // Initial renderer color
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
	// While the above seems useless on paper, it's a nice way to check if the screen is being refreshed or not
    SDL_RenderPresent(renderer);
	
	// ---------------------- Hardware ----------------------
	// Initalize LCD
	lcd = vrEmuLcdNew(LCD_WIDTH, LCD_HEIGHT, EmuLcdRomA00);
	
    // 32 KB of ROM memory (0x0000 - 0x7FFF)
	// 32 KB of RAM memory (0x8000 - 0xFFFF)
	FILE *in_file;
	if ((in_file = fopen(argv[1], "rb"))) { // read only
		// file exists
	} else {
		// If no file is found, print it on the LCD
		vrEmuLcdSendCommand(lcd, LCD_CMD_DISPLAY | LCD_CMD_DISPLAY_ON);
		vrEmuLcdSendCommand(lcd, LCD_CMD_FUNCTION | LCD_CMD_FUNCTION_LCD_2LINE | 0x10);
		vrEmuLcdSendCommand(lcd, LCD_CMD_CLEAR);
		vrEmuLcdSendCommand(lcd, LCD_CMD_SET_DRAM_ADDR | 0x43);
		vrEmuLcdWriteString(lcd, "No file found!");
		refreshLCD();
		goto stop;
	}
	// Initalize all memory
	uint8_t mem[(1<<16)];
	
	// Load ROM file into Memory
	int loadROM = 0;
	char c;
	while ((c = fgetc(in_file)) != EOF)
	{
		mem[loadROM] = c;
		loadROM++;
	
		// end-of-file related
		if (feof(in_file)) {
		  // hit end of file
		} else {
		  // some other error interrupted the read
            //fprintf(stderr, "%s\n", (message));
		}
	}

    // initialize Z80 CPU
    z80_t cpu;
    uint64_t pins = z80_init(&cpu);
	
	// reset Z80 CPU for it to be in a known state
	z80_reset(&cpu);
	
	// ---------------------- Actual Emulation ----------------------
	// run code until HALT pin (active low) goes low
	int refreshTimer = SDL_GetTicks();
	while(running) {  
		// Compare SDL_GetTicks() against refreshTimer value
		// If it's been more than 16ms, update the screen, and reset the refreshTimer
		if (SDL_GetTicks() >= refreshTimer + 16) {
			refreshLCD();
			refreshTimer = SDL_GetTicks();
		}
		
	
		// Process Keyboard Inputs
		// This code is quite messy and will likely be thrown out in favor of a proper implementation that more closely matches the Bitstream that PS/2 brings
		// Hence it's commented out until I get a hardware version of this done that I can then work into a Software implementation
		// Main reason I don't just outright delete it is because it's then easier to re-figure the stuff
		// for Interrupts and Keyboard input out easier without hours of needles Google Searching.
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				running = false;
			} /*else if(event.type == SDL_KEYDOWN) {
				
				// Handle Keyboard input
				const char *key = SDL_GetKeyName(event.key.keysym.sym);
				//printf("%s\n",key);
                if(strcmp(key, "Backspace") == 0) {
					keyboard = 8;
				} else if (strcmp(key, "Return") == 0) {
					keyboard = 13;
				} else if (strcmp(key, "Escape") == 0) {
					keyboard = 27;
					running = false;
				} else if (strcmp(key, "Space") == 0) {
					keyboard = ' ';
				} else if (strcmp(key, "CapsLock") == 0) {
					capslock = !capslock;
				} else if (key[0] > ' ') {
					if (capslock)
						keyboard = key[0];
					else
						keyboard = key[0]+32;
				}
				// Issue Interrupt
				if (keyboard != 0) {
					//pins |= Z80_INT;
					//keyboard = 0;
				} else {
					//pins &= ~Z80_INT;
				}
			}*/
		}
		
		// Check to see if Interrupt is triggered
		//if (pins & Z80_INT) {
			//printf("INT\n");
		//}
		
        // tick the CPU
        pins = z80_tick(&cpu, pins); 

        // handle memory read or write access
        if (pins & Z80_MREQ) {
            if (pins & Z80_RD) {
				// Read Instructions
                Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
				//printf("%X - %X\n", Z80_GET_DATA(pins), Z80_GET_ADDR(pins));
            }
            else if (pins & Z80_WR) {
				// Write to Memory 
                mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
            }
        } else if (pins & Z80_IORQ) { // Handle I/O Devices
			// These Device cases will probably be reworked soon, due to the Hardware changing into a more simplified form
			// Additionally, being able to pick what Peripheral corresponds to what Device Number modularly is probably a more logical approach regardless
			switch (getDevice(pins)) {
				case 0: // LCD Instruction
					if (pins & Z80_WR) {	// When the LCD is being talked to
						vrEmuLcdSendCommand(lcd, Z80_GET_DATA(pins));
					} else if (pins & Z80_RD) { // When the LCD is being polled for Data
						Z80_SET_DATA(pins, vrEmuLcdReadByte(lcd));
					}
					break;
				case 1: // Send Data to the LCD
					// Wait to simulate CPU Clock
					SDL_Delay(delayTime);
					vrEmuLcdWriteByte(lcd, Z80_GET_DATA(pins));
					break;
				case 2: // Serial I/O
					//Z80_SET_DATA(pins, keyboard);
					//if (keyboard != 0) {
					//	keyboard = 0;
					//}
					break;
				case 3: // Keyboard Input
					// Somehow emulate PS/2 Keyboard
					break;
			}
		}
    }
	
	// Used to halt the Emulator in case of an error (i.e. no ROM to execute etc.)
	stop:
    while(running) {
		// Process events
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				running = false;
			} else if(event.type == SDL_KEYDOWN) {
				const char *key = SDL_GetKeyName(event.key.keysym.sym);
				if(strcmp(key, "Q") == 0) {
					running = false;
				}                 
			}
		}
	}

    // Release resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
