/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"

int getSemaphore(key_t key, int nsems){
    int semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);  

    if(semid == -1)
        errExit("semget failed!");
    return semid;
}

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = { .sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}