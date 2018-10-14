enum RUN_OPT{

    PRINT_REG = 1,
    DO_DUMP = 4,
    PRINT_AT_END = 8,

};
void print_registers(int outfd, cpu_sim_s *cm);
void run_prog(const char *output, const char *progfile, cpu_sim_s *cm,  int opt);
