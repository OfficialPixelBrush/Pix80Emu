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
int infoFlag;
// Get the current I/O Device number
uint16_t getDevice(uint64_t pins) {
    const uint16_t addr = Z80_GET_ADDR(pins) & 0b111;
	return addr;
}

const char* decodeFlags(uint8_t flags) {
	static char textFlags[7] = "-------";
	 // Carry
	if ((flags & Z80_CF) != 0) {
		textFlags[7] = 'C';
	} else {
		textFlags[7] = 'c';
	}
	
	// Add/Subtract
	if ((flags & Z80_NF) != 0) {
		textFlags[6] = 'N'; 
	} else {
		textFlags[6] = 'n'; 
	}
	
	// Parity/Overflow
	if ((flags & Z80_VF) != 0) {
		textFlags[5] = 'P';
	} else {
		textFlags[5] = 'p';
	}	
	
	// Unused
	if ((flags & Z80_XF) != 0) {
		textFlags[4] = 'X'; 
	} else {
		textFlags[4] = 'x'; 
	}
	
	// Half-Carry
	if ((flags & Z80_HF) != 0) {
		textFlags[3] = 'H';
	} else {
		textFlags[3] = 'h';
	}
	
	// Unused
	if ((flags & Z80_YF) != 0) {
		textFlags[2] = 'X';
	} else {
		textFlags[2] = 'x';
	}
	// Zero
	if ((flags & Z80_ZF) != 0) {
		textFlags[1] = 'Z';
	} else {
		textFlags[1] = 'z';
	}
	
	// Sign
	if ((flags & Z80_SF) != 0) {
		textFlags[0] = 'S';
	} else {
		textFlags[0] = 's';
	}
	
	return textFlags;
}

int main(int argc, char **argv) {
	// Get the delayTime for slowmode in milliseconds
	delayTime = atoi(argv[2]);
	infoFlag = atoi(argv[3]);
		
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
		switch (infoFlag) {
			case 1:
				printf("AF: %04hX - BC: %04hX - DE: %04hX - HL: %04hX - ADDR: %04hX\n", cpu.af, cpu.bc, cpu.de, cpu.hl, cpu.pc);
				break;
			case 2:
				printf("%s | A: %02hX | B: %02hX - C: %02hX | D: %02hX - E: %02hX | H: %02hX - L: %02hX | ADDR: %04hX\n", decodeFlags(cpu.f), cpu.a, cpu.b, cpu.c, cpu.d, cpu.e, cpu.h, cpu.l, cpu.pc);
				break;
		}
		SDL_Delay(delayTime);

		// handle memory read or write access
		if (pins & Z80_MREQ) {
			if (pins & Z80_RD) {
				// Read Instructions
				Z80_SET_DATA(pins, mem[Z80_GET_ADDR(pins)]);
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
						printf("%u",Z80_GET_DATA(pins));
					}
					break;
			}
		}
    }
	
	// Used to halt the Emulator in case of an error (i.e. no ROM to execute etc.)
	stop:
    return 0;
}
