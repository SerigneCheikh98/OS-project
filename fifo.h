/// @file fifo.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle FIFO.

#pragma once
#include <sys/types.h>

//obtain the fifo pathname
void getFifoPathname(pid_t pid, char *pathToFifo);

// make a fifo to /tmp/dev_fifo.pid with pid = to the given pid
void makeFifo(pid_t pid, char *pathname);

//open the device's fifo
int openFifo(const char *pathname, int flag);

//remove the fifo
void rmFifo(char *pathname);


