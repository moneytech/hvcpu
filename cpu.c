/*
    Author: nilput (check license file)
    License: MIT
*/
#include <string.h> 
#include <stdlib.h> 
#include <stdio.h>
#include "data.h"
#include "log.h"
#include "cpu.h"
#include "instructions_defs.h"

instruction_sim_s instr[N_INSTR] = {0};
const char    *mnem[N_INSTR] = {0};
int instructions_initialized = 0;


void illegali(cpu_sim_s *cm){
    cm->err = 1;
    log_msgf("illegal instruction encountered at IP: %x\n", cm->cpu.IP.v);
}


inline static reg_s *g_reg(cpu_sim_s *cm, uint16_t oper)
{
    switch (oper){
        case ENC_A: return &cm->cpu.A;
        case ENC_B: return &cm->cpu.B;
        case ENC_C: return &cm->cpu.C;
        case ENC_S: return &cm->cpu.S;
        case ENC_IP: return &cm->cpu.IP;
        case ENC_FLAGS: return &cm->cpu.FLAGS;
        default:
            panic_exit("g_reg(): invalid register operand");
    }
    return NULL;

}
//used in instructions like add.sub..
static inline uint16_t rencimme_source(cpu_sim_s *cm)
{
    reg_s *r2;
    uint8_t addrmode = cm->ci.opcode & ADDRMMASK;
    uint16_t s;
    switch(addrmode){
        case IMMED_M:
            s = cm->ci.op16_2;
            break;
        case REG_M:
            r2 = g_reg(cm, cm->ci.op8_2);
            s = r2->v;
            break;
        default:
            illegali(cm);
    }
    return s;
}

static inline void setf_iff(cpu_sim_s *cm, int flag, int condition)
{
    if (condition)
        cm->cpu.FLAGS.v |= flag;
    else
        cm->cpu.FLAGS.v = cm->cpu.FLAGS.v & (~flag);
}
static inline void *vmem_addr(cpu_sim_s *cm, uint16_t address)
{
    return (void *) ( cm->mem.mem + address );
}
/*RENC : register encoded*/
//adds RENC op2 to RENC op1
void addi(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    //dest
    r1 = g_reg(cm, cm->ci.op8_1);

    setf_iff(cm, OVERFLOW_F, !((r1->v ^ s) & HIGHEST_BIT) && // operands have different signs
                          (((r1->v + s) ^ r1->v) & HIGHEST_BIT) //the result's sign is different
                                                                );
    setf_iff(cm, CARRY_F, r1->v & s & HIGHEST_BIT);
    setf_iff(cm, SIGN_F, (r1->v + s) & HIGHEST_BIT); //result is negative
    setf_iff(cm, ZERO_F, ((r1->v + s) & MAX_UINT) == 0);


    r1->v += s;
}
void subi(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    //dest
    r1 = g_reg(cm, cm->ci.op8_1);

    setf_iff(cm, CARRY_F, r1->v < s);

    //negate
    s = (~s) + 1;

    setf_iff(cm, OVERFLOW_F, !((r1->v ^ s) & HIGHEST_BIT) && // operands have different signs
                                                             //(they were not before negation)
                          (((r1->v + s) ^ r1->v) & HIGHEST_BIT) //the result's sign is different
                                                                );
    setf_iff(cm, SIGN_F, (r1->v + s) & HIGHEST_BIT); //result is negative
    setf_iff(cm, ZERO_F, ((r1->v + s) & MAX_UINT) == 0);
    r1->v += s;
}
void ori(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);
    r1->v |= s;
    setf_iff(cm, ZERO_F, r1->v == 0);
}
void xori(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);
    r1->v ^= s;
    setf_iff(cm, ZERO_F, r1->v == 0);
}
void andi(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);
    r1->v &= s;
    setf_iff(cm, ZERO_F, r1->v == 0);
}

//TODO: implement carry
void shri(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);
    r1->v >>= s;
    setf_iff(cm, ZERO_F, r1->v == 0);
}

void shli(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);
    r1->v <<= s;
    setf_iff(cm, ZERO_F, r1->v == 0);
}





void negi(cpu_sim_s *cm){
    reg_s *r1;
    r1 = g_reg(cm, cm->ci.op8_2);
    r1->v = (~r1->v) + 1;
}
void noti(cpu_sim_s *cm){
    reg_s *r1;
    r1 = g_reg(cm, cm->ci.op8_2);
    r1->v = ~(r1->v);
}


void ldxi(cpu_sim_s *cm){
    reg_s *r1, *r2;
    //r1 is dest
    r1 = g_reg(cm, cm->ci.op8_1);
    uint16_t s;
    uint8_t addrmode = cm->ci.opcode & ADDRMMASK;

    switch(addrmode){
        case IMMED_M:
            s = cm->ci.op16_2;
            break;
        case REG_M:
            r2 = g_reg(cm, cm->ci.op8_2);
            s = r2->v;
            break;
        case MEMA_M:
            s = *((uint16_t *)vmem_addr(cm, cm->ci.op16_2));
            break;
        case MEMI_M:
            r2 = g_reg(cm, cm->ci.op8_2);
            s = *((uint16_t *)vmem_addr(cm, r2->v));
            break;
        default:
            illegali(cm);
    }

    r1->v = s;
}

void stxi(cpu_sim_s *cm){
    reg_s *r1, *r2;
    uint16_t *d;
    uint8_t addrmode = cm->ci.opcode & ADDRMMASK;

    //r1 is source (unusual, but we need it to support long addresses in r2)
    r1 = g_reg(cm, cm->ci.op8_1);
    //to be read as register to (addrmode)
    switch(addrmode){
        case MEMA_M:  //absolute address
            d = vmem_addr(cm, cm->ci.op16_2);
            break;
        case MEMI_M:  //register indirect
            r2 = g_reg(cm, cm->ci.op8_2);
            d = vmem_addr(cm, r2->v);
            break;
        default:
            illegali(cm);
    }

    *d = r1->v;
}

void cmpi(cpu_sim_s *cm){
    reg_s *r1;
    uint16_t s = rencimme_source(cm);
    r1 = g_reg(cm, cm->ci.op8_1);

    setf_iff(cm, CARRY_F, r1->v < s);

    //negate
    s = (~s) + 1;

    setf_iff(cm, OVERFLOW_F, !((r1->v ^ s) & HIGHEST_BIT) && // operands have different signs
                                                             //(they were not before negation)
                          (((r1->v + s) ^ r1->v) & HIGHEST_BIT) //the result's sign is different
                                                                );
    setf_iff(cm, SIGN_F, (r1->v + s) & HIGHEST_BIT); //result is negative
    setf_iff(cm, ZERO_F, ((r1->v + s) & MAX_UINT) == 0);

}

void nopi(cpu_sim_s *cm)
{
    return;
}

static inline void do_jmp(cpu_sim_s *cm, uint16_t dest, int is_relative)
{
    if (is_relative)
        cm->cpu.IP.v += dest - sizeof(instruction_s);
    else
        cm->cpu.IP.v =  dest - sizeof(instruction_s);
}
static inline void do_step(cpu_sim_s *cm)
{
    cm->cpu.IP.v += sizeof(instruction_s);
}

void hlti(cpu_sim_s *cm){
    cm->end = 1;
}

void jcci(cpu_sim_s *cm)
{
    reg_s *f;
    f = &cm->cpu.FLAGS;
    uint16_t flg = f->v;
    uint16_t dest = cm->ci.op16_2;
    int is_relative = ((cm->ci.opcode & ADDRMMASK) == MEMI_M);

    uint8_t cond = cm->ci.op8_1;
    switch(cond){
       case JMP_JMP: 
                     do_jmp(cm, dest, is_relative);
                     return;
       case JMP_L :
                     //SF != OF
                     if ((!!(flg & SIGN_F)) != (!!(flg & OVERFLOW_F)))
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_LE:
                     if ( (flg & ZERO_F) || 
                               ((!!(flg & SIGN_F)) != (!!(flg & OVERFLOW_F))) )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_G: 
                    if ( !(flg & ZERO_F) &&
                         ((!!(flg & SIGN_F)) == (!!(flg & OVERFLOW_F))) )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_GE:
                    if ( ((!!(flg & SIGN_F)) == (!!(flg & OVERFLOW_F))) )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_A:
                    if ( (!!(flg & CARRY_F) == 0) &&
                         (!!(flg & ZERO_F)  == 0)  )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_AE:
                    if ( (!!(flg & ZERO_F)  == 0)  )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_B:
                    if ( (!!(flg & CARRY_F) == 1) )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_BE:
                    if ( (!!(flg & CARRY_F) == 1) ||
                         (!!(flg & ZERO_F)  == 0)  )
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_Z:
                    if ((flg & ZERO_F))
                        do_jmp(cm, dest, is_relative);
                    return;
       case JMP_NZ:
                    if (!(flg & ZERO_F))
                        do_jmp(cm, dest, is_relative);
                    return;
    }

    illegali(cm);
}


//stack grows downwards
void pushi(cpu_sim_s *cm)
{
    reg_s *r1, *stack;
    uint16_t *stackptr;
    stack = &cm->cpu.S;
    //source
    r1 = g_reg(cm, cm->ci.op8_2);
    stack->v -= sizeof(uint16_t);
    stackptr = vmem_addr(cm, stack->v);
    *stackptr = r1->v;
}
void popi(cpu_sim_s *cm)
{
    reg_s *r1, *stack;
    uint16_t *stackptr;
    //dest
    r1 = g_reg(cm, cm->ci.op8_2);
    stack = &cm->cpu.S;
    stackptr = vmem_addr(cm, stack->v);
    r1->v = *stackptr;
    stack->v += sizeof(uint16_t);
}

void calli(cpu_sim_s *cm){
    uint16_t target = rencimme_source(cm);
    uint16_t *stackptr;
    cm->cpu.S.v -= sizeof(uint16_t);
    stackptr = vmem_addr(cm, cm->cpu.S.v);
    *stackptr = cm->cpu.IP.v;
    cm->cpu.IP.v = target - sizeof(instruction_s);
}
void reti(cpu_sim_s *cm){
    uint16_t *stackptr;
    stackptr = vmem_addr(cm, cm->cpu.S.v);
    cm->cpu.IP.v = *stackptr;
    cm->cpu.S.v += sizeof(uint16_t);
}
void ini(cpu_sim_s *cm)
{
    /*
    reg_s *r1, *stack;
    uint16_t *stackptr;
    //dest
    */
    int input_byte = fgetc(stdin);
    if (input_byte != EOF){
        cm->cpu.A.v = (unsigned char) input_byte;
    }
    return;
}
void outi(cpu_sim_s *cm)
{
    /*
    reg_s *r1, *stack;
    uint16_t *stackptr;
    //dest
    */
    fputc(cm->cpu.A.v & 0xFF, stdout);
    fflush(stdout);
    return;
}





void do_def_instr(defined_instruction_s *df)
{
   if (df->addr_immed){
       instr[df->opcode + IMMED_M].func = df->func;
       mnem[df->opcode  + IMMED_M] = df->mnemonic;
   }
   if (df->addr_reg){
       instr[df->opcode + REG_M].func = df->func;
       mnem[df->opcode  + REG_M] = df->mnemonic;
   }
   if (df->addr_mema){
       instr[df->opcode + MEMA_M].func = df->func;
       mnem[df->opcode  + MEMA_M] = df->mnemonic;
   }
   if (df->addr_memi){
       instr[df->opcode + MEMI_M].func = df->func;
       mnem[df->opcode  + MEMI_M] = df->mnemonic;
   }
   if (df->addr_na){
       instr[df->opcode + NA_M].func = df->func;
       mnem[df->opcode  + NA_M] = df->mnemonic;
   }

}



void init_instructions()
{
    for (int i=0; i<N_INSTR; i++){
        instr[i].func = illegali;
        mnem[i] = "ILLEG";
    }
    defined_instruction_s *df = defined_instr;
    while (df->opcode != 0xFF){
        do_def_instr(df);
        df++;
    }
}

void init_cpu_sim_s(cpu_sim_s *cm)
{
    
    if (!instructions_initialized){
        init_instructions(); //global constant var
        instructions_initialized = 1;
    }
    memset(cm, 0, sizeof(cpu_sim_s));
    cm->mem.mem = malloc(SIM_MEMSIZE);
    if (!cm->mem.mem)
        goto error;
    cm->mem.size = SIM_MEMSIZE;
    cm->cycles_sec = 0;
    cm->end = 0;

    return;
error:
    cm->err = 1;
    panic_exit("init_cpu_sim_s(): no mem");
}


void destroy_cpu_sim_s(cpu_sim_s *cm)
{
    free(cm->mem.mem);

}
