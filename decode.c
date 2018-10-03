/*
Author: nilput (check license file)
License: MIT
*/
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "log.h"
#include "cpu.h"
#include "data.h"

void decode_prog(const char *output, const char *progfile)
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
            fprintf(stderr, "warning! lread!=fsize, %lld < %lld", lread, fsize);
    int dis_fd;
    dis_fd = open(output, O_RDWR | O_CREAT, 0666);
    if (dis_fd < 0){
        perror("open");
        panic_exit("couldn't open output file");
    }
    int i=0;
    instruction_s *ins;
    uint8_t opcode;
    while (i<fsize){
        ins = (instruction_s *)(content+i);
        opcode = ins->opcode;
        dprintf(dis_fd, "0x%x: %s\n", i, mnem[opcode]);
        i += sizeof(instruction_s);
        /* dprintf(dis_fd, "size: %d\n",sizeof(instruction_s)); */
    }

}
