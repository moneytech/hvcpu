#include "cpu.h"

//
//can only use the range 0x00 to 0xF8 for major opcode
//ok: 0x[0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F][8,0]
defined_instruction_s defined_instr[] = {

//  opcode     func       mnemonic    immed    reg     mema    memi   na
//INSTRBEGIN
    {0x10,     addi,      "add",      1,        1,      0,      0,    0,},
    {0x20,     subi,      "sub",      1,        1,      0,      0,    0,},
    {0x30,     xori,      "xor",      1,        1,      0,      0,    0,},
    {0x40,     andi,      "and",      1,        1,      0,      0,    0,},
    {0x50,     shri,      "shr",      1,        1,      0,      0,    0,},
    {0x60,     shli,      "shl",      1,        1,      0,      0,    0,},
    {0x80,     negi,      "neg",      1,        0,      0,      0,    0,},
    {0x90,     noti,      "not",      1,        0,      0,      0,    0,},
    {0xA0,     cmpi,      "cmp",      1,        1,      0,      0,    0,},
    {0xB0,     nopi,      "nop",      0,        0,      0,      0,    1,},
    {0xC0,     stxi,      "stx",      0,        0,      1,      1,    0,},
    {0xD0,     ldxi,      "ldx",      1,        1,      1,      1,    0,},
    {0xE0,     pushi,     "push",     0,        1,      0,      0,    0,},
    {0xF0,     popi,      "pop",      0,        1,      0,      0,    0,},
    {0xF8,     ini,       "in",       0,        0,      0,      0,    1,},
    {0xE8,     outi,      "out",      0,        0,      0,      0,    1,},
    {0xD8,     hlti,      "hlt",      0,        0,      0,      0,    1,},
    {0xB8,     jcci,      "jcc",      1,        0,      0,      1,    0,},
    {0xA8,     ori,       "or",       1,        1,      0,      0,    0,},
    {0x98,     calli,     "call",     1,        1,      0,      0,    0,},
    {0x88,     reti,      "ret",      0,        0,      0,      0,    1,},
//INSTREND
    {0xFF,     illegali,  "UNDEF",    0,        0,      0,      0,    0,},
};

