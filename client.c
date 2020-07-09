/// @file client.c
/// @brief Contiene l'implementazione del client.

/*#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/msg.h>*/

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"

//#define PATH "/tmp/message_id"

//the FIFO pathname
char pathnameToFifo[20];

/*bool valid_message_id(int message_id){
    int fd, bW = -1;

    //get the file descriptor 
    fd = open("msqid", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);

    //check if open failed
    if(fd == -1)
        errExit("open failed!");

    int bR = -1, id;

    bR = read(fd, &id, sizeof(int));

    if(bR < 0)
        errExit("read failed!");
    
    if(message_id == id)
        return false;
}

char * used_message_id(){
    //ritorna la stringa del file delle message id
    //pensare a come eliminare il file al termine del processo
}*/

int main(int argc, char * argv[]) {

    //check the input arguments
    if(argc != 2){
        printf("Usage: %s msg_queue_key\n", argv[0]);
        exit(1);
    }

    //get the msg_queue_key 
    key_t msgKey = atoi(argv[1]);

    char buffer[10];
    int length;

    //create a Message data struct
    Message message;

    //read the device's pid
    printf("pid_device: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.pid_receiver = readInt(buffer);

    //read the message_id
    printf("message_id: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.message_id = readInt(buffer);
    
    /*checking if the given message_id is valid
    if(!valid_message_id(message.message_id)){
        printf("message_id must be univocal\n\t%s\n", used_message_id());
        exit(1);
    }*/

    //read the message to send
    printf("message: ");
    fgets(message.message, sizeof(message.message), stdin);
    length = strlen(message.message);
    message.message[length - 1] = '\0';

    //read max distance
    printf("max_distance (must be positive):" );
    fgets(buffer, sizeof(buffer), stdin);
    message.max_distance = readDouble(buffer);

    
    //preparing the message...
    //get the client's pid
    message.pid_sender = getpid();
    
    //get the pathname of the FIFO
    getFifoPathname(message.pid_receiver, pathnameToFifo);

    //open the file descriptor in write only mode
    int fd_fifo = open(pathnameToFifo, O_WRONLY); 
    
    if(fd_fifo == -1)
        errExit("open FIFO failed!");

    //write the message in the FIFO
    int bW = -1;
    
    bW = write(fd_fifo, &message, sizeof(message));
    
    if(bW != sizeof(message))
        errExit("write failed!");
    
    //close the FIFO
    close(fd_fifo);
    
    //wait the acknowledgment list
    //get the message queue identifier
    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);

    if(msqid == -1)
        errExit("msgget failed!");
    
    //message structure 
    Msq_struct msq_struct;

    //get the size of the message
    size_t mSize = sizeof(Msq_struct) - sizeof(long);

    //wait the acknowledge list with mtype equals to message_id
    printf("waiting the acknowlegment list...\n");
    if(msgrcv(msqid, &msq_struct, mSize, message.message_id, 0) == -1)
        errExit("msgrcv failed!");
    
    printf("\n ack_list received\n");
    for(int j = 0; j < 5 ; j++)
        printf("%5d, pidS: %d, pidR: %d\n", msq_struct.ack_list[j].message_id, msq_struct.ack_list[j].pid_sender, msq_struct.ack_list[j].pid_receiver);
    printf("\n");

    //print the list
    printAckList(&msq_struct, message);
    
    //termination
    exit(0);
}