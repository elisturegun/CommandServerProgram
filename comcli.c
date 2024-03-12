
#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>

#include <sys/stat.h>
#include "shareddefs.h"


char *bufferp; // send buffer
int   bufferlen; // buffer size  (in bytes)
char csPipePath[16];
char scPipePath[16];
mqd_t mq;
char* MQNAME;

void sig_handler(int signum){
    close(cspipe);
    close(scpipe);
    remove(csPipePath);
    remove(scPipePath);
    mq_unlink(MQNAME);
    printf("Client QUITALL successful.\n");
    exit(0);
}

struct mqmessage* clientMssg;


int WSIZE = 1; // used if size input not given

int main(int argc, char* argv[])
{

    signal(SIGUSR1,sig_handler);
    int batchFlag = 0;
    if(argc == 2){
        printf("Only MQNAME argument given\n");
    }
    else if(argc == 4){
        printf("MQNAME && COMFILE || WSIZE arguments given\n");
        if(strcasecmp(argv[2],"-b") == 0){
            batchFlag = 1;
        }
        else if(strcasecmp(argv[2],"-s") == 0){
            WSIZE = strtol(argv[3],NULL,10);
        }
        else{
            printf("Error giving arguments.\n");
            exit(-1);
        }
    }
    else if(argc == 6){
        printf("All MQNAME, COMFILE, and WSIZE arguments given\n");
        if(strcasecmp(argv[2],"-b") == 0 && strcasecmp(argv[4],"-s") == 0){
            batchFlag = 1;
            WSIZE = strtol(argv[5],NULL,10);
        }
        else{
            printf("Error giving arguments.\n");
            exit(-1);
        }
    }
    else{
        printf("Wrong number of inputs. Quitting program.\n");
        exit(-1);
    }

    int quitFlag = 0;
    int quitALLFlag = 0;

    pid_t cid = getpid(); // store id
    // Define unique pathname
    sprintf(csPipePath,"cspipe%d",cid);
    sprintf(scPipePath,"scpipe%d",cid);
    printf("%s\n",scPipePath);
    printf("%s\n",csPipePath);

    cspipe = mkfifo(csPipePath, 0666);
    if(cspipe < 0){
        printf("Could not create cs pipe\n");
        exit(1);
    }
    scpipe = mkfifo(scPipePath, 0666);
    if(scpipe < 0){
        printf("Could not create sc pipe\n");
        exit(1);
    }


    MQNAME = argv[1];

    //mqd_t mq;
    struct mq_attr mq_attr;

    // allocate specified size to struct's data
    //struct message* m = malloc(sizeof (struct message) + WSIZE);
    
    mq = mq_open(MQNAME, O_RDWR);
    if (mq == -1) {
        perror("can not open msg queue\n");
        exit(1);
    }
    printf("mq opened, mq id = %d\n", (int) mq);

    mq_getattr(mq,&mq_attr);
    mq_attr.mq_msgsize = (long) malloc(sizeof(struct mqmessage));
    mq_getattr(mq,&mq_attr);
    printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);
    bufferlen = mq_attr.mq_msgsize;
    bufferp = (char *) malloc(bufferlen);

    clientMssg = (struct mqmessage *) bufferp;
    mqWriteWsize(clientMssg, WSIZE);
    clientMssg->type[0] = '0';
    clientMssg->padding[0] = 000;
    sprintf(clientMssg->data, "%d ", cid);

    printf("message size: %d\n", mqReadWsize(clientMssg));
    printf("client id: %s\n", clientMssg->data);

    // Send the connection request message through message queue
    int n = mq_send(mq, bufferp, sizeof(struct mqmessage), 0);
    if (n == -1){
        perror("Client connection message failed\n");
        exit(1);
    }

    printf("Client connection message sent\n");

    int messageBufferLength = sizeof (struct message);// + WSIZE * sizeof (char );
    struct message recv_mssg;
    struct message sent_mssg;

    // Get and print message sent by the server
    scpipe = open(scPipePath, O_RDONLY);
    read(scpipe, &recv_mssg, messageBufferLength);
    char* mainServerID = strtok(recv_mssg.data,"&");
    char* childServerID = strtok(NULL,"&");
    printf("MAIN SERVER: %s. Connected to child server %s\n",mainServerID ,childServerID );
    close(scpipe);
    pid_t sid = strtol(recv_mssg.data,NULL,10);
    // Clean message
    cleanMessage(&recv_mssg);

    char* commandBuffer;
    FILE* inputFile;
    if(batchFlag){
        inputFile = fopen(argv[3], "r");
        if (inputFile == NULL){
            printf("Batch input file failed to be opened\n");
            exit(-1);
        }
    }
    do {
        // Clean message
        cleanMessage(&sent_mssg);

        if(batchFlag){
            char inputCommand[64];
            fscanf(inputFile,"%[^\n]%*c",inputCommand);
            commandBuffer = inputCommand;
            sprintf(sent_mssg.data,"%s",commandBuffer);
        }
        else{
            // Get input from user and put it into outgoing message data
            printf("Enter command: ");
            fgets(sent_mssg.data, 1024,stdin);
            // Get rid of trailing newline character
            sent_mssg.data[strcspn(sent_mssg.data,"\n")] = 0;
            commandBuffer = sent_mssg.data;
        }
        // CHECK EXIT CONDITIONS
        if(strcasecmp(commandBuffer, "quit") == 0){
            quitFlag = 1;
            sent_mssg.type[0] = '4';
            sprintf(sent_mssg.data,"%d",cid);
        }
        else if(strcasecmp(commandBuffer,"quitall") == 0){
            quitALLFlag = 1;
            sent_mssg.type[0] = '6';
            sprintf(sent_mssg.data,"%d",cid);
            kill(sid,SIGUSR1);
        }
        else{
            if(strchr(sent_mssg.data,'|') != NULL){
                sent_mssg.type[0] = '9';
            }
            else{
                sent_mssg.type[0] = '2';
            }
        }
        // Send server command to execute
        cspipe = open(csPipePath, O_WRONLY);
        write(cspipe, &sent_mssg, messageBufferLength);
        fflush(stdout);
        close(cspipe);
        // Get server output
        scpipe = open(scPipePath, O_RDONLY);
        read(scpipe, &recv_mssg, messageBufferLength);
        printf("%s\n",recv_mssg.data);
        close(scpipe);
        // Clean message
        cleanMessage(&recv_mssg);
    } while (!quitFlag && !quitALLFlag);

    mq_close(mq);
    remove(csPipePath);
    remove(scPipePath);
    printf("Client %d quitting.\n",cid);
    return 0;
}
