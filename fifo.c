/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "err_exit.h"
#include "fifo.h"


void getFifoPathname(pid_t pid, char *pathToFifo){
    //char pathToFifo[16];

    sprintf(pathToFifo, "/tmp/dev_fifo.%d", pid );

    //return pathToFifo;
}


void makeFifo(pid_t pid, char *pathname){ 
    //char *pathname = getFifoPathname(pid);

    //make the fifo
    if (mkfifo(pathname, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("<Device> mkfifo failed");

    printf("<Device> FIFO %s created!\n", pathname);
    fflush(stdout);
}


int openFifo(const char *pathname, int flag){
    //get the file descriptor of the fifo
    int fd = open(pathname, flag);

    if(fd == -1)
        errExit("openFifo failed!");
    
    printf("fifo opened successfully\n");
    fflush(stdout);
    
    return fd;           
}

void rmFifo(char *pathname){
    printf("removing %s fifo", pathname);
   
    if(unlink(pathname) == -1)
        errExit("unlink failed!");
}