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

; pause process with whatever PID has been provided
; last address of the program has been pushed to stack
; must be called via a CALL to push address
pauseProcess:
DI ; disable interrupts
; IX points to the current Process' header
PUSH HL ; push hl to stack
INC SP
INC SP  ; go up a step on the stack
POP BC  ; load PC into HL
; write PC to header
LD (IX+0), H
LD (IX+1), L
; load the og HL back in
DEC SP
DEC SP
DEC SP
DEC SP ; go back to where HL is stored
POP HL ; put HL back where it belongs

; go back to before the pc was pushed
INC SP
INC SP
INC SP
INC SP

; write all dem registers to the header
LD (IX+2), A
; TODO: prolly faster using stack operations for this!!
; e.g.  put the stack pointer here temporarily, then dump the data,
;       then move it back to where it was
; TODO: Doing thats suggested above is necessary,
;       since we can't put the stack on there otherwise lol
LD A,(IX) ; get process address
EI
JP selectNextProcess 

; resume process with whatever PID has been provided
; must be called via a JMP to not change SP
; PID to be resumed provided by A
resumeProcess:
DI
LD ($8000),A
LD A, 0
LD ($8001),A
LD IX, ($8000) ; transfer Process Header Pointer to IX
LD H,(IX+0)
LD L,(IX+1)
PUSH HL ; prep return address
LD A,(IX+2) ; prep A register
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