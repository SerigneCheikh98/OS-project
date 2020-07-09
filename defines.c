/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progett
#include "defines.h"
#include "err_exit.h"

int readInt(const char *s) {
    char *endptr = NULL;

    errno = 0;
    long int res = strtol(s, &endptr, 10);

    if (errno != 0 || *endptr != '\n' || res < 0) {
        printf("invalid input argument\n");
        exit(1);
    }

    return res;
}

double readDouble(const char *s) {
    char *endptr = NULL;

    errno = 0;
    double res = strtod(s, &endptr);

    if (errno != 0 || res <= 0) {
        printf("invalid input argument\n");
        exit(1);
    }

    return res;
}

void printAckList(Msq_struct *msq_struct, Message message){
    char pathname[15];
    int fd, bW = -1;

    //creating the pathname 
    sprintf(pathname, "out_%d.txt", msq_struct->ack_list[0].message_id);

    //get the file descriptor of the output file
    fd = open(pathname, O_CREAT | O_RDONLY | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);

    //check if open failed
    if(fd == -1)
        errExit("open failed!");

    //first and second line
    char buff[305];
    sprintf(buff, "Messaggio %d: %s \nLista acknowledgment:\n", message.message_id, message.message);
    
    if(write(fd, &buff, strlen(buff) * sizeof(char)) == -1){
        printf("write failed!\n");
    } 
    
    //printing the body of the received ack_list
    int counter = 0; 
    char  text[100];
    
    do{
        sprintf(text, "%d, %d, %ld \n", msq_struct->ack_list[counter].pid_sender, msq_struct->ack_list[counter].pid_receiver, msq_struct->ack_list[counter].timestamp);
        // Writing up to size bytes on the output file
        bW = write(fd, &text, strlen(text) * sizeof(char));

        //checking if write successed
        if(bW != strlen(text) * sizeof(char))
            errExit("write failed!");

        counter++;    
    }while (counter < 5);
    
    //close the output file
    close(fd);
}

/***********************************************************SERVER***********************************************************/
void send_rm_ack(int msqid, Msq_struct *ack_list_message, Acknowledgement *ptr_ack , int maxIndex){
    //get the size of the message
    int mSize = sizeof(Msq_struct) - sizeof(long);
    int i, c = 0;

    for(i = 0; i <= maxIndex; i++){
        if(ptr_ack[i].message_id == ack_list_message->mtype){
            //set the message's ack_list
            ack_list_message->ack_list[c] = ptr_ack[i];
            c++;
        
            //remove the ack from the ack_list
            ptr_ack[i].message_id = -1;
        }
    }

    //send the ack_list to the client
    if(msgsnd(msqid, ack_list_message, mSize, 0) == -1)
        errExit("msgsnd failed!");
}

