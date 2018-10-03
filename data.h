#ifndef SIMDATA_H
#define SIMDATA_H

#include <sys/types.h>
#include <stdint.h>

#define N_INSTR 256
#define HIGHEST_BIT 0x8000

typedef struct _reg_s{
    union{
        uint16_t v;
        int16_t sv;
    };
} reg_s;

enum FLAGS_F{
    CLEAR_F=1,
    CARRY_F=2,
    OVERFLOW_F=4,
    SIGN_F=8,
    ZERO_F=16,
};

/*
0xF     0xF
0000|0     000
^inst(0xF8) ^adrrmode (0x7)
 */
/*
 all instructions are 32bit wide
 without using any fancy extensions
 there is space for only 32 instructions in the format, with 8 possible variants of each for addressing mode 
 instructions end with: 
immed: 0x?0 OR 0x?8
reg:   0x?1 OR 0x?9
mem:   0x?2 or 0x?A
 */
//and the opcode with ADDRMMASK to get these
#define ADDRMMASK 0x7
enum ADDR_MODE{

    NA_M = 0x0,         /* not applicable */
    IMMED_M = 0x1,      /* immediate op16 */
    REG_M = 0x2,        /* RENC */
    MEMA_M = 0x3,       /* memory absolute op16 */
    MEMI_M = 0x4,       /* memory indirect through RENC */ /* second meaning: for jmps relative to instruction */

};

/*
   register encoding:
 * */
enum REG_ENC{
    ENC_A      = 1,
    ENC_B      = 2,
    ENC_C      = 3,
    ENC_S      = 4,
    ENC_IP     = 5,
    ENC_FLAGS  = 6,
};
/*
   jmp conditions:
 * */
enum JMP_COND{
    JMP_JMP   = 0x1, //unconditional
    JMP_L     = 0x2,
    JMP_LE    = 0x3,
    JMP_G     = 0x4,
    JMP_GE    = 0x5,
    JMP_A     = 0x6,
    JMP_AE    = 0x7,
    JMP_B     = 0x8,
    JMP_BE    = 0x9,
    JMP_Z     = 0xA, //JE
    JMP_NZ    = 0xB, //JNE
};

typedef struct _cpu_s{
    reg_s A;
    reg_s B;
    reg_s C;
    reg_s S;
    reg_s IP;
    reg_s FLAGS;
} cpu_s;


typedef struct _mem_s{
    uint8_t *mem;
    uint16_t size;

} mem_s;

typedef struct _instruction_s{
    union{
        struct{
            uint8_t opcode;
            uint8_t  op8_1;
            uint16_t  op16_2;
        };
        struct{
            uint8_t _opcode;
            uint8_t _op8_1; 
            uint8_t  op8_2;
            uint8_t  op8_3; //unused
        };
    };
} instruction_s;

typedef struct _cpu_sim_s{
    cpu_s cpu;
    mem_s mem;
    //current instruction
    instruction_s ci;
    int cycles_sec;
    int single_step;
    int err;
} cpu_sim_s;


typedef void (*instr_fx) (cpu_sim_s *cm);

typedef struct _instruction_sim_s{
    instr_fx func;
} instruction_sim_s;

#endif
