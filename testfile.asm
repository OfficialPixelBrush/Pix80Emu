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

; IX contains the pointer to the start of the Program's memory
; the first 256 bytes of a program are just it's header info
createProcess:
RET

; pause process with whatever PID has been provided
; last address of the program has been pushed to stack
pauseProcess:
POP HL
DEC SP
DEC SP
LD A,H
OUT (32),A
LD A,L
OUT (32),A
; push IX to stack
;PUSH IX
; return stack to where it was before
;INC SP
;INC SP
; snatch PC

;PUSH AF
;PUSH BC
;PUSH DE
;PUSH HL
;PUSH IX
;PUSH IY
RET

; resume process with whatever PID has been provided
resumeProcess:
RET

; deallocate and unload process with provided PID
killProcess:
RET
