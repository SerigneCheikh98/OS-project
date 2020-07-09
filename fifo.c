/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "err_exit.h"
#include "fifo.h"


void getFifoPathname(pid_t pid, char *pathToFifo){
    sprintf(pathToFifo, "/tmp/dev_fifo.%d", pid);
}


void makeFifo(pid_t pid, char *pathname){ 
    //make the fifo
    if (mkfifo(pathname, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("<Device> mkfifo failed");
}


void rmFifo(char *pathname){
    printf("removing %s fifo\n", pathname);
    fflush(stdout);
   
    if(unlink(pathname) == -1)
        errExit("unlink failed!");
}