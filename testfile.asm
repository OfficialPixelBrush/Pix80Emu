; in permanent RAM, we keep track of the number of currently alive Programs, their IDs and Pointers + Mem Bank Setting (4 bytes)
; ID , Bank, Hi Pointer, Lo Pointer
; big array of which 256-byte memory block belongs to which process!!
; pointer to a progress consists of ID (high byte)
; since the segmented area is only 14-bit,
; those top two extra bits might be used to give each process up to 64k of memory

; process' are given however many chunks are given by their segment header
; a process can have up to 256 chunks, if available
; at the last address alloated to the process, all the registers are stored
; after those, the stack resides, it
; the PID is the first one that's been marked as "empty"
; the pointer is determined based on the PID

; --- Program Header --- 
; PID (1 byte)
; Number of Chunks (1 byte)
; SP, AF, BC, DE, HL, IX, IY (2+2+2+2+2+2+2+2 = 32 bytes)
; 32 Byte Header

; first byte is number of active process'
processListing EQU 0x9000

LD A,0
LD (processListing), A
loop:
    LD HL, programOne
    LD A,H
    OUT (32),A
    CALL createProcess
    LD HL, programTwo
    LD A,H
    OUT (32),A
    CALL createProcess
    JP selectNextProcess
HALT
;JP loop

selectNextProcess:
    ; go into here with ID of last process
    CP 1 ; if last ID was 1
    JR Z, resumeSecond
    JR resumeFirst
    resumeFirst:
        LD A, 1
        JP resumeProcess
    resumeSecond:
        LD A, 2
        JP resumeProcess
HALT

createProcess:
    LD A,(processListing)
    INC A
    LD (processListing), A
RET

; pause process with whatever PID has been provided via A
; last address of the program has been pushed to stack
; must be called via a CALL to push address
pauseProcess:
DI

EI
JP selectNextProcess 

; resume process with whatever PID has been provided
; must be called via a JMP to not change SP
; PID to be resumed provided by A
resumeProcess:
DI
EN
RET

; deallocate and unload process with provided PID
killProcess:
JP selectNextProcess

ORG 0x0100
programOne:
    ; print from 'A' until 'E'
    LD A,'A'
    loopOne:
        CALL pauseProcess ; pause Process to go to other process
        OUT (32),A
        INC A
        CP 'E'
        JP C,loopOne
    HALT
    ;RET

ORG 0x0200
programTwo:
    ; print from 'U' until 'X'
    LD A,'T'
    loopTwo:
        CALL pauseProcess ; pause Process to go to other process
        OUT (32),A
        INC A
        CP 'Z'
        JP Z,loopTwo
    HALT
    ; Best case the application would be killed here
    ;RET