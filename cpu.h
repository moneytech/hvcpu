#include "data.h"

#define SIM_MEMSIZE (0xFFFF)
#define MAX_UINT    (0xFFFF)
void init_cpu_sim_s(cpu_sim_s *cm);

extern instruction_sim_s instr[];
extern const char    *mnem[];

