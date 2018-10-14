/*
Author: nilput (check license file)
License: MIT
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "data.h"
#include "log.h"
#include "cpu.h"
#include "decode.h"
#include "run.h"

void help(const char *msg);
int main(int argc, const char **argv)
{
    int c;
    int got_opt = 0;
    const char *in = NULL;
    const char *out = NULL;

    cpu_sim_s cm;
    init_cpu_sim_s(&cm);
    cm.cycles_sec = 0;
    int dis = 0;
    int run = 0;
    int cycle_print = 0;
    int end_print = 1;
    int do_dump = 0;
    int set_cycles = 0; //no limit

    opterr = 0;

    while ((c = getopt (argc,  (char * const *)argv, "o:d:r:c:vmh")) != -1)
        switch (c)
        {
        case 'o':
            out = optarg;
            break;
        case 'd':
            in = optarg;
            dis = 1;
            break;
        case 'r':
            in = optarg;
            run = 1;
            break;
        case 'c':
            set_cycles = atoi(optarg);
            if (set_cycles <= 0){
                panic_exit("cant set cycles to <= 0");
            }
            break;
        case 'v':
            cycle_print = 1;
            break;
        case 'e':
            end_print = 0;
            break;
        case 'm':
            do_dump = 1;
            break;
        case 'h':
            help(NULL);
            return 1;
        case '?':
            if (optopt == 'd')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,"Unknown option character `\\x%x'.\n",optopt);

            help(NULL);
            return 1;
            default:
                help(NULL);
        }

    int run_opt = 0x0;
    if (cycle_print){
        run_opt = run_opt | PRINT_REG;
    }
    if (do_dump){
        run_opt = run_opt | DO_DUMP;
    }
    if (end_print){
        run_opt = run_opt | PRINT_AT_END;
    }
    cm.cycles_sec = set_cycles;
    
    if (dis){
        if(!in)
            help("expected an input file");
        if(!out)
            out = "/dev/fd/1";
        decode_prog(out, in);
        got_opt = 1;

    }
    else if (run){
        if(!in)
            help("expected an input file");
        if(!out)
            out = "/dev/fd/1";

        run_prog(out, in, &cm,  run_opt);
        got_opt = 1;
    }
    if (!got_opt){
        help("no options specified");
    }

}

void help(const char *msg)
{
    if (!msg)
        msg = "";

    printf( "%s\n"
            "vcpu 0.1\n"
            "   options:\n"
            "       -o [file] outfile\n"
            "       -d [file] dissassmeble\n"
            "       -c [cycles] limit number of cycles/sec\n"
            "       -v print register info with each cycle\n"
            "       -e dont print register at the end\n"
            "       -r [file] run\n", msg);
    exit(1);
}
