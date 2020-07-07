/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

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

void printAckList(Msq_struct *msq_struct){
    char *pathname = "";
    int fd, bW = -1;

    //creating the pathname 
    sprintf(pathname, "out_%d", msq_struct->ack_list[0].message_id);

    //get the file descriptor of the output file
    fd = open(pathname, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);

    //check if open failed
    if(fd == -1)
        errExit("open failed!");

    //chunk of Acknowledgeement
    size_t size = sizeof(Acknowledgement);  
    
    int counter = 0;
    char newLine = '\n';
    
    do{
        // Writing up to size bytes on the output file
        bW = write(fd, &msq_struct->ack_list[counter], size);

        //checking if write successed
        if(bW != size)
            errExit("write failed!");
        
        //write a new line for the next Acknowledgement
        write(fd, &newLine, sizeof(char));

        counter++;    
    }while (counter < 5);
    
    //close the output file
    close(fd);
}

/***********************************************************SERVER***********************************************************/
void send_rm_ack(int msqid, Msq_struct *ack_list_message, Acknowledgement *ptr_ack , int maxIndex){
    printf("<Ack_manager>sending ack_list to client...\n");
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

