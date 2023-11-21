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
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 
  
#define CHIPS_IMPL
#include "./include/z80.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
//#include <SDL2/SDL.h>

// Utility macros
#define CHECK_ERROR(test, message) \
    do { \
        if((test)) { \
            fprintf(stderr, "%s\n", (message)); \
            exit(1); \
        } \
    } while(0)

bool running = true;
//SDL_Event event;
int delayTime;
unsigned char infoFlag = 0;
int currentBank = 0;
char latestKeyboardCharacter;
uint16_t addr;
z80_t cpu;

// Initalize all memory
uint8_t onBoardROM[(1<<14)] = { 0 };
// Initalize all memory
uint8_t onBoardRAM[(1<<15)] = { 0 };

const char* decodeFlags(uint8_t flags) {
	// 8 because it stores 8 chars, 0 indexed
	static char textFlags[8] = "-------";
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

void printDebugInfo() {
    switch (infoFlag) {
	    case 1:
		    printf("AF: %04hX - BC: %04hX - DE: %04hX - HL: %04hX - ADDR: %04hX BANK:%02hX\n", cpu.af, cpu.bc, cpu.de, cpu.hl, cpu.pc, currentBank);
		    break;
	    case 2:
		    printf("%s | A: %02hX | B: %02hX - C: %02hX | D: %02hX - E: %02hX | H: %02hX - L: %02hX | ADDR: %04hX BANK:%02hX\n", decodeFlags(cpu.f), cpu.a, cpu.b, cpu.c, cpu.d, cpu.e, cpu.h, cpu.l, cpu.pc, currentBank);
		    break;
	    default:
		    break;
    }
}

uint8_t readMappedMemory(uint16_t address) {
    if (address < 0x4000) {
        // Constant ROM
		return onBoardROM[address];
    } else if (address < 0x8000) {
        // Banking Area
		// Check with the I/O Object how it's memory is set up
		return 0;
    } else if (address >= 0x8000) {
        // Constant RAM
		return onBoardRAM[address-0x8000];
    }
    // Outside of mapable memory!
    return 0;
}

uint8_t writeMappedMemory(uint16_t address, uint8_t data) {
    if (address < 0x4000) {
        // Constant ROM
		// Can't write to ROM :^)
    } else if (address < 0x8000) {
        // Banking Area
		// Check with the I/O Object how it's memory is set up
		return 0;
    } else if (address >= 0x8000) {
        // Constant RAM
		onBoardRAM[address-0x8000] = data;
    }
    // Outside of mapable memory!
    return 0;
}

int main(int argc, char **argv) {
	// Get the delayTime for slowmode in milliseconds
	// delayTime = atoi(argv[2]);
	//infoFlag = atoi(argv[2]);
		
    // 32 KB of ROM memory (0x0000 - 0x7FFF)
	// 32 KB of RAM memory (0x8000 - 0xFFFF)
	FILE *in_file;
	if ((in_file = fopen(argv[1], "rb"))) { // read only
		// file exists
		printf("Loading ROM from %s\n",argv[1]);
		// Find out filesize
		fseek(in_file, 0L, SEEK_END);
		int filesize = ftell(in_file);
		rewind(in_file);
		// Print file info
		printf("File is 0x%04hX Bytes large\n",filesize);
	} else {
		printf("No file found!\n");
		return 1;
	}
	
	// Load ROM file into Memory
	size_t bytes_read = 0;
	bytes_read = fread(onBoardROM, sizeof(unsigned char), 0x4000, in_file);
	printf("ROM of size 0x%04hX/0x4000 was loaded\n",(int)bytes_read);

    // initialize Z80 CPU
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
		
		// Debug Info
		if (infoFlag) {
		    printDebugInfo();
		}
		//SDL_Delay(delayTime);

		// handle memory read or write access
        addr = Z80_GET_ADDR(pins);
		if (pins & Z80_MREQ) {
			if (pins & Z80_RD) {
				// Read Instructions
				Z80_SET_DATA(pins, readMappedMemory(addr));
			}
			else if (pins & Z80_WR) {
				// If writing to memory
                writeMappedMemory(addr,Z80_GET_DATA(pins));
			}
		} else if (pins & Z80_IORQ) { // Handle I/O Devices
		    // Might make use of the fact
		    // the B register does shit too another time lmao
		    switch(addr & 0xFF) {
		        // Memory Bank Selector
		        case 0b00000000:
		            if (pins & Z80_WR) {
		                currentBank = Z80_GET_DATA(pins);
		            }
		            break;
		        // Most likely where the Serial Port will be
		        case 0b00100000:
		            if (pins & Z80_WR) {
		                putchar(Z80_GET_DATA(pins));
		            }
		            break;
		        default:
		            if (infoFlag) {
		                printf("Unassigned Device!");
		            }
		            break;
		    }
		}
    }
	
	// Used to halt the Emulator in case of an error (i.e. no ROM to execute etc.)
	return 0;
}
