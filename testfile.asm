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
tempStackLocation EQU 0x8000
numberOfProcesses EQU 0x8002
currentProcess EQU 0x8003

; init
LD A,0
LD (numberOfProcesses), A
LD (currentProcess), A
; IM 2 ; set interrupt mode 2 

; create programs
    LD HL, programOne
    LD A,H
    OUT (32),A
    CALL createProcess
    LD HL, programTwo
    LD A,H
    OUT (32),A
    CALL createProcess
    JR selectNextProcess
    ; go to process selection
;JP loop

ORG 0x0038
selectNextProcess:
    LD A,(currentProcess)
    CP 1 ; if last ID was 1
    JR Z, resumeSecond
    JR resumeFirst
    resumeFirst:
        LD A, 1
        JP resumeProcess
    resumeSecond:
        LD A, 2
        JP resumeProcess
HALT ; something went horribly wrong lol

createProcess: ; increment number of process'
    EI
    LD A,(numberOfProcesses)
    INC A
    LD (numberOfProcesses), A
    DI
RET

; pause currentProcess
pauseProcess:
; Stolen from
; https://domipheus.com/blog/teensy-z80-homebrew-computer-part-5-implementing-preemptive-multithreading/
DI
PUSH HL
PUSH BC
PUSH AF
PUSH DE
PUSH IX
PUSH IY
LD (tempStackLocation), SP
EXX         ; TIL that the shadow registers
EX AF,AF'   ; aren't mirrors of the normal regs 
JP selectNextProcess 

; resume process with whatever PID has been provided via currentProcess
resumeProcess:
LD A,(currentProcess)
; load data for given process
EX AF,AF'
EXX
LD SP,(tempStackLocation) ; maybe handle this var via PID?
POP IY
POP IX
POP DE 
POP AF
POP BC
POP HL
RET ; crashes here

; deallocate and unload process with provided PID
killProcess:
JP selectNextProcess

ORG 0x0100
programOne:
    ; print from 'A' until 'E'
    LD A,'A'
    loopOne:
        OUT (32),A
        INC A
        CP 'E'
        CALL pauseProcess ; pause Process to go to other process
        JP C,loopOne
    HALT
    ;RET

ORG 0x0200
programTwo:
    ; print from 'U' until 'X'
    LD A,'T'
    loopTwo:
        OUT (32),A
        INC A
        CP 'Z'
        CALL pauseProcess ; pause Process to go to other process
        JP Z,loopTwo
    HALT
    ; Best case the application would be killed here
    ;RET