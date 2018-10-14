/*
Author: nilput (check license file)
License: MIT
*/
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "log.h"
#include "cpu.h"
#include "data.h"
#include "run.h"

const char *current_instr(cpu_sim_s *cm)
{
    return mnem[cm->ci.opcode];
}

void print_registers(int fd, cpu_sim_s *cm)
{
    const char *carrf =  (cm->cpu.FLAGS.v & CARRY_F)      ? "CARRY" : "";
    const char *overf =  (cm->cpu.FLAGS.v & OVERFLOW_F)   ? "OVERF" : "";
    const char *signf =  (cm->cpu.FLAGS.v & SIGN_F)       ?  "SIGN "  : "";
    const char *zerof =  (cm->cpu.FLAGS.v & ZERO_F)       ?  "ZERO "  : "";

    dprintf(fd,  "\n"
                 "A:  0x%.4hx:%.5hd   B: 0x%.4hx:%.5hd      \n"
                 "C:  0x%.4hx:%.5hd   S: 0x%.4hx:%.5hd      \n"
                 "IP: 0x%.4hx:%.5hd                         \n"
                 "FLAGS: [%5s]  [%5s]  [%5s] [%5s]          \n"
                 "Current instruction: %s                   \n",
                 cm->cpu.A.v,cm->cpu.A.sv,       cm->cpu.B.v,cm->cpu.B.sv,
                 cm->cpu.C.v,cm->cpu.C.sv,       cm->cpu.S.v,cm->cpu.S.sv,
                 cm->cpu.IP.v,cm->cpu.IP.sv,
                 carrf, overf, signf, zerof,
                 current_instr(cm) );
}

void run_prog(const char *output, const char *progfile, cpu_sim_s *cm,  int opt)
{
    int fd;
    ssize_t fsize, lread;
    char *content;
    fd = open(progfile, O_RDONLY);
    if (fd < 0){
        perror("open");
        panic_exit("couldn't open the file");
    }
    fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    content = malloc(fsize);
    if (!content){
        panic_exit("readprog: no mem");
    }
    lread = read(fd, content, fsize);
    if (lread != fsize)
            fprintf(stderr, "warning! lread!=fsize, %lld != %lld", lread, fsize);
    if (lread > cm->mem.size){
            fprintf(stderr, "error, program is larger than addressable memory\n");
            fprintf(stderr, "program size: %lld, addressable memory: %lld \n", lread, cm->mem.size);
            panic_exit("");
    }

    memcpy(cm->mem.mem, content, lread);
    free(content);
    content = NULL;
    close(fd);

    int dis_fd;
    dis_fd = open(output, O_RDWR | O_CREAT, 0666);
    if (dis_fd < 0){
        perror("open");
        panic_exit("couldn't open output file");
    }



    instruction_s *ins;
    uint8_t opcode;
    while (1){
        ins = (instruction_s *)(cm->mem.mem + cm->cpu.IP.v);
        memcpy(&cm->ci, ins, sizeof(instruction_s));
        opcode = ins->opcode;
        if (opt & PRINT_REG)
            print_registers(dis_fd, cm);
        instr[opcode].func(cm);

        cm->cpu.IP.v += sizeof(instruction_s);

        if (cm->err || cm->end){
            break;
        }
        if (cm->cycles_sec){
            usleep(1e6 / cm->cycles_sec);
        }
    }
    if (cm->err){
        fprintf(stderr, "\nan error was encountered, IP: 0x%.4hx", cm->cpu.IP.v);
        print_registers(2,cm);
    }
    if (opt & DO_DUMP){
        int fd = open("dump.bin", O_WRONLY | O_CREAT | O_TRUNC);
        if(fd >= 0){
            write(fd, cm->mem.mem, cm->mem.size);
            close(fd);
        }
        else{
            fprintf(stderr, "couldn't open file to dump mem\n");
        }
    }
    if (opt & PRINT_AT_END){
        print_registers(2,cm);
    }

}
