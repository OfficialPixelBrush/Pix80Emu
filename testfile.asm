loop:
LD A,'A'
PUSH AF
LD A,'B'
POP AF
OUT (32),A
JP loop
