/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include "err_exit.h"
#include "shared_memory.h"

int getSharedMemory(key_t key, size_t size){
    // get, or create, a shared memory segment
    int shmid = shmget(key, size, IPC_CREAT | S_IRUSR | S_IWUSR);
   
    if(shmid == -1)
        errExit("shmget failed!");
        
    return shmid;
}

void *attach(int shmid, int shmflg){
    // attach the shared memory
    void *ptr_sh = shmat(shmid, NULL, shmflg);

    if(ptr_sh == (void *)-1)
        errExit("shmat failed!");

    return ptr_sh;
}

void detach(const void *ptr_sh){
    // detach the shared memory segments
    if(shmdt(ptr_sh) == -1)
        errExit("shmdt failed!");
}

void rmshm(int shmid){
    // delete the shared memory segment
    if(shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("IPC_RMID failed!");
}