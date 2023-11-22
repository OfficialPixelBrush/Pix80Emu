; in permanent RAM, we keep track of the number of currently alive Programs, their IDs and Pointers + Mem Bank Setting (4 bytes)
; ID , Bank, Hi Pointer, Lo Pointer

; process' are given however many chunks are given by their segment header
; a process can have up to 256 chunks, if available
; at the last address alloated to the process, all the registers are stored
; after those, the stack resides, it
; the PID is the first one that's been marked as "empty"
; the pointer is determined based on the PID

loop:
CALL pauseProcess
JP loop

createProcess:

RET

; pause process with whatever PID has been provided
; last address of the program has been pushed to stack
; must be called via a CALL to push address
pauseProcess:
; disable interrupts during this
DI
PUSH SP
PUSH AF
PUSH BC
PUSH DE
PUSH HL
PUSH IX
PUSH IY
; reenable them
EI
RET

; resume process with whatever PID has been provided
; must be called via a JMP to not change SP
resumeProcess:
DI
POP IY
POP IX
POP HL
POP DE
POP BC
POP AF
POP SP
EN
RET

; deallocate and unload process with provided PID
killProcess:
RET
