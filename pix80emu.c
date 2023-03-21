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

#define CHIPS_IMPL
#include <z80.h>

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

bool running = true;
SDL_Event event;
int delayTime;
// Get the current I/O Device number
uint16_t getDevice(uint64_t pins) {
    const uint16_t addr = Z80_GET_ADDR(pins) & 0b111;
	return addr;
}

int main(int argc, char **argv) {
	// Get the delayTime for slowmode in milliseconds
	delayTime = atoi(argv[2]);
		
    // 32 KB of ROM memory (0x0000 - 0x7FFF)
	// 32 KB of RAM memory (0x8000 - 0xFFFF)
	FILE *in_file;
	if ((in_file = fopen(argv[1], "rb"))) { // read only
		// file exists
	} else {
		printf("No file found!");
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
	//int refreshTimer = SDL_GetTicks();
	while(running) {
        // tick the CPU
		// Wait to simulate CPU Clock
		pins = z80_tick(&cpu, pins); 
		SDL_Delay(delayTime);

		// handle memory read or write access
		if (pins & Z80_MREQ) {
			if (pins & Z80_RD) {
				// Read Instructions
				Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
				//printf("AF: %04hX - BC: %04hX - DE: %04hX - HL: %04hX - ADDR: %04hX\n", cpu.af, cpu.bc, cpu.de, cpu.hl, cpu.pc);
			}
			else if (pins & Z80_WR) {
				// Write to Memory 
				if (Z80_GET_ADDR(pins) > 0x7FFF) {
					mem[Z80_GET_ADDR(pins)] = Z80_GET_DATA(pins);
				} else {
					printf("Trying to access ROM\n");
				}
			}
		} else if (pins & Z80_IORQ) { // Handle I/O Devices
			// These Device cases will probably be reworked soon, due to the Hardware changing into a more simplified form
			// Additionally, being able to pick what Peripheral corresponds to what Device Number modularly is probably a more logical approach regardless
			switch (getDevice(pins)) {
				case 0: // Send to screen
					if (pins & Z80_WR) {	// When the LCD is being talked to
						printf("%c",Z80_GET_DATA(pins));
					}
					break;
			}
		}
    }
	
	// Used to halt the Emulator in case of an error (i.e. no ROM to execute etc.)
	stop:
    return 0;
}
