#ifndef V_CPU_H
#define V_CPU_H 

#include "data.h"

#define SIM_MEMSIZE (0xFFFF)
#define MAX_UINT    (0xFFFF)
void init_cpu_sim_s(cpu_sim_s *cm);



//used for populating instr array
typedef struct _defined_instruction_s{
    uint8_t opcode;
    instr_fx func;
    const char *mnemonic;
    //whether its defined for the address modes
    char addr_immed;
    char addr_reg;
    char addr_mema;
    char addr_memi;
    char addr_na;
} defined_instruction_s;






void addi(cpu_sim_s *cm);     
void subi(cpu_sim_s *cm);     
void xori(cpu_sim_s *cm);     
void andi(cpu_sim_s *cm);     
void shri(cpu_sim_s *cm);     
void shli(cpu_sim_s *cm);     
void negi(cpu_sim_s *cm);     
void noti(cpu_sim_s *cm);     
void cmpi(cpu_sim_s *cm);     
void nopi(cpu_sim_s *cm);     
void stxi(cpu_sim_s *cm);     
void ldxi(cpu_sim_s *cm);     
void pushi(cpu_sim_s *cm);    
void popi(cpu_sim_s *cm);     
void ini(cpu_sim_s *cm);      
void outi(cpu_sim_s *cm);     
void hlti(cpu_sim_s *cm);     
void jcci(cpu_sim_s *cm);     
void ori(cpu_sim_s *cm);      
void calli(cpu_sim_s *cm);    
void reti(cpu_sim_s *cm);     
void illegali(cpu_sim_s *cm); 

extern instruction_sim_s instr[];
extern const char    *mnem[];

#endif 
