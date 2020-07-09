/// @file server.c
/// @brief Contiene l'implementazione del SERVER.

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"


#define BOARD 5

#define BOARD_DIM 100
#define ACK_LIST_DIM 100

//shared memory segments identifiers
int SHM_BOARD;
int SHM_ACK;

//semaphore set for the acknowledgement list
int SEM_IDX_ACK;
int SEM_IDX_BOARD;

//message queue identifier
int MSQ_ACK_MANAGER_TO_CLIENTS;

//pathname of the device's fifo
char path2FIFO[20];

//fifo descriptor
int fd_FIFO;
//int fd_FIFOextra;

//file descriptor of file_posizioni.txt
int fd_positions;

//copy of file_posizioni.txt
char buffer[1000];

//step counter
int STEP = 0;

//array containing devices pid
pid_t pid_devices[5];
//pid of the task_manager
pid_t pid_ack_manager;

//pointers to shared memory segments
pid_t *ptr_board;
Acknowledgement *ptr_ack;

/*******************************************server_sigHandler*****************************************************/
void sigHandler(int sig){
    int i, j, k;
    if(sig == SIGTERM){   
        i = 0;   
        
        printf("Terminating the ack_manager...\n");
        //termination of the task manager
        if(kill(pid_ack_manager, SIGTERM) == -1)
            errExit("Termination of the task_manager failed!");        
        
        //termination of devices
        printf("Terminating devices...\n");
        while(i < 5){
            if(kill(pid_devices[i], SIGTERM) == -1)
                errExit("Termination a device failed!");
            i++;
        }
        
        printf("removing semaphors...\n");
        //remove the SEM_IDX_ACK semaphore set 
        if(semctl(SEM_IDX_ACK, 0, IPC_RMID, 0) == -1)
            errExit("IPC_RMID of SEM_IDX_ACK failed!");
        
        //remove the SEM_IDX_BOARD semaphore set 
        if(semctl(SEM_IDX_BOARD, 0, IPC_RMID, 0) == -1)
            errExit("IPC_RMID of SEM_IDX_BOARD failed!");

        printf("Detaching and removind both shared memory segments...\n");
        //detach both shared memory segments
        detach(ptr_board);
        detach(ptr_ack);

        //remove both shared memory segments
        rmshm(SHM_BOARD);
        rmshm(SHM_ACK);

        //positions
        if(close(fd_positions) == -1)
            errExit("close of file_posizioni.txt failed!");
        //termination
        exit(0);
    }
    else if(sig == SIGUSR1){
        while (1){
            //wait before the next step
            sleep(2);

            /*************************************print status of devices*************************************/
            bool notSended;

            //access the ack_list
            semOp(SEM_IDX_ACK, 0, -1);

            //first row
            printf("\n#Step %d: devices positions ####################\n", STEP);
            for (i = 0; i < 5; i++){
                printf("%d %c %c msgs: ", pid_devices[i], buffer[(i * 4) + (STEP * 20)], buffer[(i * 4) + 2 + (STEP * 20)]);
                //print the list of message_id
                for (j = 0; j < ACK_LIST_DIM; j++){
                    notSended = true;
                    if (ptr_ack[j].message_id == -1){
                        continue;
                    }
                    if (ptr_ack[j].pid_receiver == pid_devices[i]){
                        //check if it already sended or not
                        for (k = 0; k < ACK_LIST_DIM; k++){

                            if (ptr_ack[k].pid_sender == pid_devices[i] && ptr_ack[k].message_id == ptr_ack[j].message_id){
                                notSended = false;
                                break;
                            }
                        }

                        if (notSended == true)
                            printf("%d ", ptr_ack[j].message_id);
                    }
                }
                printf("\n");
            }
            //free the ack_list
            semOp(SEM_IDX_ACK, 0, 1);

            printf("################################################\n");

            printf("******************************************************\n");
            for (int i = 0; i < BOARD_DIM; i++){
                if (i % 10 == 0)
                    printf("\n");
                printf("%5d  ", ptr_board[i]);
            }
            printf("\n******************************************************\n");
            
            //next step
            STEP++;
            //unlock the first device
            semOp(SEM_IDX_BOARD, 0, 1);
        }
    }
}


/*******************************************ack_manager_sighandler*****************************************************/
void ack_manager_sighandler(int sig){
    //coda di messaggi
    if (msgctl(MSQ_ACK_MANAGER_TO_CLIENTS, IPC_RMID, NULL) == -1)
        errExit("IPC_RMID of the msg_queue failed!");
    
    //termination
    exit(0);
}

/*******************************************devices_sighandler*****************************************************/
void devices_sighandler(int sig){
    //close of fifo
    if(close(fd_FIFO) == -1)
        errExit("<Device> close of fifo failed!");

    //removing the fifo  
    rmFifo(path2FIFO);
    
    //termination
    exit(0);
}

/*void taskServer(){
    int i, j, k;
    while(1){
        printf("<Server> starting the next STEP...\n");
        //wait before the next step
        sleep(4);

        *************************************print status of devices*************************************        
        bool notSended;
        
        //first row
        printf("\n#Step %d: devices positions ####################\n\n", STEP);
        //access the ack_list
        semOp(SEM_IDX_ACK, 0, -1);
        for(i = 0; i < 5; i++){
            printf("%d %c %c msgs: ", pid_devices[i], buffer[(i * 4) + (STEP * 20)], buffer[(i * 4) + 2 + (STEP * 20)]); 
            //print the list of message_id
            for(j = 0; j < ACK_LIST_DIM; j++){
                notSended = true;
                if(ptr_ack[j].message_id == -1){
                    continue;
                }
                if(ptr_ack[j].pid_receiver == pid_devices[i]){
                    
                    //check if it already sended or not
                    for(k = j + 1; k < ACK_LIST_DIM; k++){
                        
                        if(ptr_ack[k].pid_sender == pid_devices[i] && ptr_ack[k].message_id == ptr_ack[j].message_id){
                            notSended = false;
                            break;
                        }  
                    }

                    if(notSended == true)
                       printf("%d ",ptr_ack[j].message_id);
                }
            }
            printf("\n");           
        }
        //free the ack_list
        semOp(SEM_IDX_ACK, 0, 1);
        
        printf("################################################\n");

        printf("******************************************************\n");
        for (int i = 0; i < BOARD_DIM; i++)
        {
            if(i % 10 == 0)
                printf("\n");
            printf("%5d  ", ptr_board[i]);
            
        }
        printf("\n******************************************************\n");
        //next step
        STEP++;

        //unlock the first device
        semOp(SEM_IDX_BOARD, 0, 1);
    }
}*/

int main(int argc, char * argv[]) {
    
    //check command line arguments
    if(argc != 3){
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    //get the msg_queue_key
    key_t msgKey = atoi(argv[1]);

    //get the pathname of the file with device's positions
    char *pathToFilePositions;
    pathToFilePositions = argv[2];
    /************************************************************SIGNALS************************************************************/
    //set of signals
    sigset_t set;
    //initialize the signal set
    sigfillset(&set);
    //delete SIGTERM from the set
    sigdelset(&set, SIGTERM);
    //delete SIGUSR1 from the set
    sigdelset(&set, SIGUSR1);/*************************************************************************/
   
    //block all signals except SIGTERM & SIGUSR1
    if(sigprocmask(SIG_BLOCK, &set, NULL) == -1)
        errExit("SIG_BLOCK failed!");

    //change signals handler
    if(signal(SIGTERM, sigHandler) == SIG_ERR || signal(SIGUSR1, sigHandler) == SIG_ERR)
        errExit("<Server> change signal handler failed!");

    /************************************************************SEMAPHORS************************************************************/
    //get the acknowledgement list semaphore set 
    SEM_IDX_ACK = semget(IPC_PRIVATE, 1, IPC_CREAT | S_IRUSR | S_IWUSR);  
    if(SEM_IDX_ACK == -1)
        errExit("semget failed!");
    
    unsigned short semAckInitVal[] = {1};
    union semun argAck_list;
    argAck_list.array = semAckInitVal;

    //initialize the semaphore set
    if(semctl(SEM_IDX_ACK, 0, SETALL, argAck_list) == -1){
        errExit("SETALL of SEM_IDX_ACK failed!");
    }

    //get the board's and device's semaphore set
    SEM_IDX_BOARD = semget(IPC_PRIVATE, 6, IPC_CREAT | S_IRUSR | S_IWUSR);  

    if(SEM_IDX_BOARD == -1)
        errExit("semget failed!");

    //initialize the semaphore set
    unsigned short semBoardInitVal[] = {1, 0, 0, 0, 0, 1};
    union semun argBoard;
    argBoard.array = semBoardInitVal;

    if(semctl(SEM_IDX_BOARD, 0, SETALL, argBoard) == -1){
        errExit("SETALL of SEM_IDX_BOARD failed!");
    };

    /************************************************************SHARED MEMORY********************************************************/
    /***************************BOARD******************************/ 
    //board of pids....an array as matrix
    pid_t board[BOARD_DIM];
    
    //get the identifier of the shared memory segment
    SHM_BOARD = getSharedMemory(IPC_PRIVATE, sizeof(board));

    //attach the board shared memory segment 
    ptr_board = (pid_t *) attach(SHM_BOARD, 0);

    //initialize the board
    for(int i = 0; i < BOARD_DIM; i++){
        ptr_board[i] = 0;
    }

    /**************************ACK_LIST***************************/
    //get the identifier of the acknowledgement list shm
    SHM_ACK = getSharedMemory(IPC_PRIVATE, sizeof(Acknowledgement) * ACK_LIST_DIM);

    //attach the acknowledgement list shared memory segment
    ptr_ack = (Acknowledgement *) attach(SHM_ACK, 0); 

    //initialize the ack_list
    for(int i = 0; i < ACK_LIST_DIM; i++){
        ptr_ack[i].message_id = -1;
    }
    /*********************************************************************************************************************************/
    //generate the ack_manager
    pid_ack_manager = fork();
    
    //server and ack_manager are both here! 
    if(pid_ack_manager == -1)
        errExit("fork for ack_manager failed!"); 
    
    if(pid_ack_manager == 0){
    /***********************************************************ACK_MANAGER***********************************************************/
        //change SIGTERM handler
        if(signal(SIGTERM, ack_manager_sighandler) == SIG_ERR)
            errExit("<Ack_manager> change signal handler failed!");
        
        //get the message queue for the communication between the ack_manager and clients
        MSQ_ACK_MANAGER_TO_CLIENTS = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
        
        if(MSQ_ACK_MANAGER_TO_CLIENTS == -1)
            errExit("msgget of the msq between ack_manager & clients failed!");
        
        int i, j, counter;

        //declaration of the ack_list message
        Msq_struct ack_list_message; 

        while(1){
            //sleep for 5 seconds before checking again the acknowledge list 
            sleep(5);
            //access the Acknowledge list
            semOp(SEM_IDX_ACK, 0, -1);

            //check the i-th mesagge_id
            for(i = 0; i < ACK_LIST_DIM; i++){
                counter = 0;
                if(ptr_ack[i].message_id != -1){
                //check if the i-th message_id is ready to be sent
                    for(j = i; j < ACK_LIST_DIM; j++){
                        if(ptr_ack[j].message_id != -1 && ptr_ack[j].message_id == ptr_ack[i].message_id){
                            counter++;
                            //check if i reached 5 acks 
                            if(counter == 5){
                                //send the message only to the client who sent it
                                ack_list_message.mtype = ptr_ack[i].message_id;
                                //send the msq message and remove ack from the ack_list 
                                send_rm_ack(MSQ_ACK_MANAGER_TO_CLIENTS, &ack_list_message, ptr_ack, j);
                                //exit the iteration
                                break;
                            }
                        }
                    }
                }
            }
            //unlock the semaphore
            semOp(SEM_IDX_ACK, 0, 1);
        } 
    }
    /*********************************************************************************************************************************/
    
    /***************************FILE_POSIZIONI******************************/
    ssize_t bR;

    //get the file_positioni.txt in read only mode
    fd_positions = open(pathToFilePositions, O_RDONLY);
    //check if open failed
    if(fd_positions == -1)
        errExit("open of file_poisizioni.txt failed!");
    
    //read of 50 rows of the file...50 steps
    bR = read(fd_positions, buffer, 999);
    //check if read failed
    if (bR == -1)
        errExit("read of file_posizioni.txt failed!");

    buffer[bR] = '\0';
    /**********************************************************************/
    //generation of devices
    int device;
    for(device = 0; device < 5; device++){
        pid_devices[device] = fork();
       
        //server and device_i are both here! 
        if(pid_devices[device] == -1)
            errExit(("creation of device failed!"));
        
        else if(pid_devices[device] == 0){
        /***********************************************************DEVICES***********************************************************/
            int i, j, k, x, y, write_fifo, bW = -1;;
            //change SIGTERM handler
            if(signal(SIGTERM, devices_sighandler) == SIG_ERR)
                errExit("<Device> change signal handler failed!");

            /***************************FIFO******************************/
            //get the pathname
            getFifoPathname(getpid(), path2FIFO);

            //create the device's fifo
            makeFifo(getpid(), path2FIFO);

            //get the fifo descriptor            
            fd_FIFO = open(path2FIFO, O_RDONLY | O_NONBLOCK);

            if(fd_FIFO == -1)
                errExit("openFifo failed!");

            //array of device's internal messages
            Message fifo_messages[20];

            //initialize the array
            for(i = 0; i < 20; i++)
                fifo_messages[i].message_id = -1;

            while (1){ 
                //unlock the first device
                semOp(SEM_IDX_BOARD, device, -1);

                //assuming that there is not messages at the first STEP
                if(STEP == 0){
                    //get index
                    x = (buffer[(device * 4)] - '0') * 10;
                    y = (buffer[(device * 4) + 2] - '0');

                    //access the board
                    semOp(SEM_IDX_BOARD, (unsigned short) BOARD, -1);

                    //set the position
                    ptr_board[x + y] = getpid();

                    //free the board
                    semOp(SEM_IDX_BOARD, (unsigned short) BOARD, 1);
                }
                else{
                    /***************************sending messages******************************/
                    double distance;
                    pid_t pidSender;

                    //access the ack_list
                    semOp(SEM_IDX_ACK, 0, -1);
                    //access the board
                    semOp(SEM_IDX_BOARD, BOARD, -1);

                    bool alreadyReceived;
                    //check if there are messages to send
                    for(i = 0; i < 20; i++){
                        if(fifo_messages[i].message_id != -1){
                            //check there are devices within the Max_distance
                            for(j = 0; j < BOARD_DIM; j++){
                                alreadyReceived = false;
                                if(ptr_board[j] != 0){
                                   
                                    //get index 
                                    x = ((buffer[(device * 4)] - '0') - (j/10) - (buffer[(device * 4)] - '0') - (j/10)) * ((buffer[(device * 4)] - '0') - (j/10) - (buffer[(device * 4)] - '0') - (j/10));
                                    y = ((buffer[(device * 4) + 2] - '0') - (j%10) - (buffer[(device * 4) + 2] - '0') - (j%10)) * ((buffer[(device * 4) + 2] - '0') - (j%10) - (buffer[(device * 4) + 2] - '0') - (j%10));
                                    
                                    distance = 3/*sqrt(x + y)*/;
                                    //printf("\ndistance: %f\n", distance);

                                    if(distance <= fifo_messages[i].max_distance){
                                        //check if the device has already received this message
                                        for(k = 0; k < ACK_LIST_DIM; k++){
                                            if(ptr_ack[k].message_id == fifo_messages[i].message_id && ptr_ack[k].pid_receiver == ptr_board[j]){
                                                alreadyReceived = true;
                                                break;
                                            }
                                        }

                                        if(!alreadyReceived){
                                            //set the message
                                            pidSender = getpid();
                                            fifo_messages[i].pid_sender = pidSender;
                                            fifo_messages[i].pid_receiver = ptr_board[j];
                                            
                                            //send the message 
                                            getFifoPathname(ptr_board[j], path2FIFO);
                                            
                                            write_fifo = open(path2FIFO, O_WRONLY);

                                            if(write_fifo == -1)
                                                errExit("openFifo failed!");

                                            bW = write(write_fifo, &fifo_messages[i], sizeof(fifo_messages[i]));

                                            if(bW == -1)
                                                errExit("its look like the fifo is broken!");
                                            else if(bW != sizeof(fifo_messages[i]))
                                                errExit("write failed!");

                                            //close the device's fifo
                                            close(write_fifo);

                                            //delete the message
                                            fifo_messages[i].message_id = -1;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //leave the board
                    semOp(SEM_IDX_BOARD, BOARD, 1);
                    /***************************receiving messages******************************/
                    
                    Message message;
                    bR = -1;
                        
                    //check if there are messages on the fifo
                    bR = read(fd_FIFO, &message, sizeof(message));
                    
                    if(bR == -1)
                        errExit("<Device> its look like my fifo is broken");
                    else if(bR == sizeof(message)){
                        //write the ack at the first available position
                        for(i = 0; i < ACK_LIST_DIM; i++){
                            if(ptr_ack[i].message_id == -1){
                                ptr_ack[i].pid_sender = message.pid_sender;
                                ptr_ack[i].pid_receiver = getpid();
                                ptr_ack[i].message_id = message.message_id;
                                ptr_ack[i].timestamp = time(NULL);
                                break;
                            }
                        }
     
                        for(i = 0; i < 20; i++){
                            //insert the message at the first available position in the internal list
                            if(fifo_messages[i].message_id == -1){
                                fifo_messages[i] = message;
                                break;
                            }
                        }    

                        int counter = 0;
                        //check if i'm the last receiver of this message
                        for(j = 0; j < ACK_LIST_DIM; j++){
                            if(ptr_ack[j].message_id == message.message_id){
                                counter++;
                                //check if i reached 5 acks 
                                if(counter == 5){
                                    //delete the message
                                    fifo_messages[i].message_id = -1;
                                    //exit the iteration
                                    break;
                                }
                            }
                        }      
                    }

                    //free the ack_list
                    semOp(SEM_IDX_ACK, 0, 1);

                    /***************************movement_on_the_board******************************/
                    //access the board
                    semOp(SEM_IDX_BOARD, (unsigned short) BOARD, -1);

                    //reset the previous position
                    x = (buffer[(device * 4) + ((STEP - 1) * 20)] - '0') * 10;
                    y = (buffer[(device * 4) + 2 + ((STEP - 1) * 20)] - '0');

                    //reset
                    ptr_board[x + y] = 0;

                    //get index
                    x = (buffer[(device * 4) + (STEP * 20)] - '0') * 10;
                    y = (buffer[(device * 4) + 2 + (STEP * 20)] - '0');

                    //set the  next position
                    ptr_board[x + y] = getpid();

                    //free the board
                    semOp(SEM_IDX_BOARD, (unsigned short) BOARD, 1);
                }
                
                if (device != 4){
                    //unlock the next device
                    semOp(SEM_IDX_BOARD, (unsigned short) device + 1, 1);
                    STEP++;
                }
                else{
                    STEP++;
                    //notify the server to start the next STEP
                    kill(getppid(), SIGUSR1);
                }
                
            }
        }
    }

    //only SERVER run here!

    //taskServer();
    //wait until all children terminates or until SIGTERM signal has been delivered 
    while(wait(NULL) != -1);
}