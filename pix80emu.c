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
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
// #include <SDL2/SDL.h>

#define ROMSTART  0x0000
#define ROMEND    0x7FFF
#define RAMSTART  0x8000
#define RAMEND    0xFFFF

#define BANKSTART 0x4000
#define BANKEND   0x7FFF

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
int currentBank = 0;
char latestKeyboardCharacter;
uint16_t addr;

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

int main(int argc, char **argv) {
	// Get the delayTime for slowmode in milliseconds
	delayTime = atoi(argv[2]);
	infoFlag = atoi(argv[3]);
		
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
		printf("No file found!");
		goto stop;
	}
	// Initalize all memory
	uint8_t mem[(1<<16)] = { 0 };
	uint8_t banks[1<<3][(1<<14)] = { 0 };
	
	// Load ROM file into Memory
	size_t bytes_read = 0;
	bytes_read = fread(mem, sizeof(unsigned char), 0x7FFF, in_file);
	printf("ROM of size 0x%04hX/0x7FFF was loaded\n",(int)bytes_read);

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
		
		/*
		if (pins & Z80_HALT) {
			printf("HALT ");
		}
		
		if (pins & Z80_INT) {
			printf("INT ");
		}
		
		if (pins & Z80_NMI) {
			printf("NMI ");
		}*/
		
		
		// Debug Info
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
		SDL_Delay(delayTime);

		// TODO: Handle I/O Memory Banking
		// handle memory read or write access
        addr = Z80_GET_ADDR(pins);
		if (pins & Z80_MREQ) {
			if (pins & Z80_RD) {
				// Read Instructions
				//printf("Read from %04hX\n",addr);
				// If Adress is in banking, load banked mem, if not IO 0
				if ((addr >= BANKSTART) && (addr <= BANKEND) && (currentBank != 0)) {
					Z80_SET_DATA(pins, banks[currentBank][addr&0x3FFF]);
				} else {
					Z80_SET_DATA(pins, mem[addr]);
				}

			}
			else if (pins & Z80_WR) {
				// If writing to memory
				//printf("Wrote to %04hX\n",addr);
				int addr = Z80_GET_ADDR(pins);
				// Check if we're in a memory bank instead of main memory
				if ((addr >= BANKSTART) && (addr <= BANKEND) && (currentBank != 0)) {
					banks[currentBank][addr&0x3FFF] = Z80_GET_DATA(pins);
				} else {
					// If we're within regular memory, make sure we can't write to ROM
					if (addr >= RAMSTART) {
						mem[addr] = Z80_GET_DATA(pins);
					} else {
						// printf("Failed trying to write to ROM\n");
					}
				}
			}
		} else if (pins & Z80_IORQ) { // Handle I/O Devices
			if (pins & Z80_M1) {
				// an interrupt acknowledge cycle, depending on the emulated system,
				// put either an instruction byte, or an interrupt vector on the data bus
				Z80_SET_DATA(pins, 0x08);
				pins &= ~Z80_INT;
			}
			// Update Bank if A7 is high during IORQ
			if (addr & 0b10000000) {
				currentBank = ((addr & 0b01110000) >> 4);
			}
			// Do IO Stuff
			switch (addr & 0b01110000) {
				case 0b00000000:
					if (pins & Z80_WR) {
						printf(BYTE_TO_BINARY_PATTERN"\n",BYTE_TO_BINARY(Z80_GET_DATA(pins)));
					}
					break;
				case 0b00010000:
					// Terminal
					if (pins & Z80_WR) {
						printf("%c",Z80_GET_DATA(pins));
					}
					if (pins & Z80_RD) {
						Z80_SET_DATA(pins,latestKeyboardCharacter);
					}
					if (pins & Z80_WR) {
						printf("%c",Z80_GET_DATA(pins));
					}
					break;
				case 0b00100000:
					if (pins & Z80_WR) {
						printf("%02hX",Z80_GET_DATA(pins));
					}
					break;
				/*
				default:
					printf("Invalid IO Device %02hX\n",(addr & 0b01110000));
					break;
				*/
			}
		}
		
		/// Interrupts are to be issued here
		// Keyboard interrupt
        if (kbhit()) {
            // fetch typed character into latestKeyboardCharacter
            latestKeyboardCharacter = getch();
			//printf("Keyboard Hit: %c\n", latestKeyboardCharacter);
			pins |= Z80_INT;
        } 
    }
	
	// Used to halt the Emulator in case of an error (i.e. no ROM to execute etc.)
	stop:
    return 0;
}
