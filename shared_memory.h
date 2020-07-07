/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

//get the shared memory segment identifier 
int getSharedMemory(key_t key, size_t size);

//attach the shared memory segment identified bys shmid to the calling process’s virtual address space
void *attach(int shmid, int shmflg);

//detach ther shared memory segment
void detach(const void *ptr_sh);

//remove the shared memory segment identified by shmid
void rmshm(int shmid);