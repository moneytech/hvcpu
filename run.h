enum RUN_OPT{

    PRINT_REG = 1,
    IGNORE_CYCLE = 2,
    DO_DUMP = 4,

};
void print_registers(int outfd, cpu_sim_s *cm);
void run_prog(const char *output, const char *progfile, cpu_sim_s *cm,  int opt);
