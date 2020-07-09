/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
/*#ifndef DEFINES_H
#define DEFINES_H*/

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/msg.h>

typedef struct{
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    char message[256];
    double max_distance;
} Message;

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
} Acknowledgement;

typedef struct {
    long mtype;
    Acknowledgement ack_list[5];
} Msq_struct;

//read integers from char pointer
int readInt(const char *s);

//read double from char pointer
double readDouble(const char *s);

//print the acknowledgement list on the out_message_id file
void printAckList(Msq_struct *msq_struct, Message message);

//send the message and remove acks from the ack_list
void send_rm_ack(int msqid, Msq_struct *ack_list_message, Acknowledgement *ptr_ack, int maxINdex);


//#endif