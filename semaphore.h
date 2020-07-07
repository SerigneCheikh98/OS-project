/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once
#include <sys/sem.h>
#include <fcntl.h>

// definition of the union semun
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};

//get semaphore set identifier
int getSemaphore(key_t key, int nsems);

//perform operations
void semOp (int semid, unsigned short sem_num, short sem_op);
