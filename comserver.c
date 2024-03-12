#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "shareddefs.h"

char *bufferp;  // pointer to receive buffer - allocated with malloc()
int bufferlen; // length of the receive buffer
struct item *itemp;
struct mqmessage* connectionRequest;
pid_t clients[MAX_CLIENT_NUM];
pid_t child_servers[MAX_CLIENT_NUM];
int clientCounter = 0;

void printMQStatus(struct mqmessage* mqmssg){
    int wsize = mqReadWsize(mqmssg);
    int cid;
    cid = strtol(mqmssg->data, NULL, 10);
    printf("\n\n********** STATUS REPORT **********\n");
    printf("\nmain server: CONREQUEST message received: client id:%d, cs=cspipe%d sc=scpipe%d, wsize=%d\n\n",cid,cid,cid,wsize);
    printf("********** ------------- **********\n\n");
    return;
}

void printStatus(struct message* mssg){
    int type = strtol(mssg->type, NULL, 10);
    int wsize = mssgReadWsize(mssg);
    int cid;
    printf("\n\n********** STATUS REPORT **********\n");
    switch (type) {
        case 0: // CONREQUEST
            cid = strtol(mssg->data, NULL, 10);
            printf("\nERROR. CONREQUEST received through named pipe. type:%d, wsize:%d,client id: %d, data:\n%s\n\n",type,wsize,cid,mssg->data);
            break;

        case 1: // CONREPLY
            printf("\nchild server: CONREPLY message sent. Message data:\n%s\n\n", mssg->data);
            break;

        case 2: // COMLINE
            printf("\nchild server: COMLINE message received. Message data:\n%s\n\n", mssg->data);
            break;

        case 3: // COMRESULT
            printf("\nchild server: COMRESULT message sent. Message data:\n%s\n\n", mssg->data);
            break;

        case 4: // QUITREQ
            printf("\nchild server: QUITREQ message received. Message data:\n%s\n\n",mssg->data);
            break;

        case 5: // QUITREPLY
            printf("\nchild server: QUITREPLY message sent. Message data:\n%s\n\n",mssg->data);
            break;

        case 6: // QUITALL
            printf("\nchild server: QUITALL message received. Message data:\n%s\n\n", mssg->data);
            break;

        case 8: // COMPOUND RESULT
            printf("\nchild server: COMPOUND-COMRESULT message sent. Message data:\n%s\n\n", mssg->data);
            break;

        case 9: // COMPOUND COMMAND
            printf("\nchild server: COMPOUND-COMLINE message received. Message data:\n%s\n\n", mssg->data);
            break;
    }
    printf("********** ------------- **********\n\n");
    return;
}

void sig_handler(int signum){
    for (int i = 0; i < MAX_CLIENT_NUM; ++i) {
        kill(child_servers[i], SIGUSR1);
        kill(clients[i], SIGUSR1);
    }
    close(cspipe);
    close(scpipe);
    remove("comcli");
    remove("comserver");
    printf("QUITALL SUCCESS\n");
    exit(0);
}

int main(int argc, char* argv[])
{
    signal(SIGUSR1,sig_handler);
    if(argc != 2){
        printf("Wrong number of inputs, only MQNAME is implemented so far.");
    }

    pid_t sid = getpid(); // store id
    printf("Server id is:%d\n",sid);

    char* MQNAME = argv[1];
    mqd_t mq;
    struct mq_attr mq_attr;
    int n;
    pid_t mainsid = getpid();
    int quitALLFlag = 0;

    mq = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL); // Create message queue and wait for message
    if (mq == -1) {
        perror("can not create msg queue\n");
        exit(1);
    }

    mq_getattr(mq, &mq_attr);
    printf("mq created, mq id = %d\n", (int) mq);
    printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

    //allocate large enough space for the buffer to store
    //an incoming message
    bufferlen = mq_attr.mq_msgsize;
    bufferp = (char *) malloc(bufferlen);
    
    while (!quitALLFlag) {
        // Listen for connection request
        n = mq_receive(mq, bufferp, bufferlen, NULL);

        if (n == -1) {
            perror("mq_receive failed\n");
            exit(1);
        }
        // Connection request message received
        connectionRequest = (struct mqmessage*) bufferp;

        // Allocate enough size for message
        int WSIZE = mqReadWsize(connectionRequest);

        //struct message* m = malloc(sizeof (struct message) + WSIZE);

        // print status
        printMQStatus(connectionRequest);

        // Get mqmessage content
        int cid = strtol(connectionRequest->data, NULL, 10);

        char csPipePath[12];
        char scPipePath[12];
        sprintf(csPipePath,"cspipe%d",cid);
        sprintf(scPipePath,"scpipe%d",cid);

        // Add client to clients array
        clients[clientCounter] = cid;

        // set message size for messages through the pipe
        int messageBufferLength = sizeof (struct message);// + WSIZE * sizeof (char );

        struct message recv_mssg;
        struct message sent_mssg;

        if(1){
            sid = fork(); // message has been received, fork to let child server handle the message
            if(sid == -1){
                printf("Server child couldn't be created\n");
            }
            else if(sid == 0){
                //! ********* CHILD SERVER CODE *********
                //printf("Inside child server\n");
                pid_t csid = getpid();
                // Add child server id to array
                child_servers[clientCounter] = csid;
                // Send connection accepted message over sc pipe
                sent_mssg.type[0] = '1';
                //sprintf(sent_mssg.data,"Connection request of client %d accepted.\n", cid);
                sprintf(sent_mssg.data,"%d&%d", mainsid,csid);
                // server opens sc pipe
                scpipe = open(scPipePath, O_WRONLY);
                if(scpipe == -1){
                    printf("sc pipe failed to be opened\n");
                    exit(-1);
                }
                // server sends connection accept message and closes pipe
                write(scpipe, &sent_mssg, messageBufferLength);
                //fflush(stdout);
                scpipe = close(scpipe);
                // Print status
                printStatus(&sent_mssg);
                // clean message
                cleanMessage(&sent_mssg);
                do {
                    // Clean recv message
                    cleanMessage(&recv_mssg);
                    // server child opens cs pipe and configures
                    cspipe = open(csPipePath, O_RDONLY);
                    if(cspipe == -1){
                        printf("cs pipe failed to be opened\n");
                        exit(-1);
                    }
                    // reads client request
                    read(cspipe, &recv_mssg,messageBufferLength);
                    close(cspipe);
                    //printf("%s\n",recv_mssg.data);
                    //printf("The message type:%s\n",recv_mssg.type);
                    // print status
                    printStatus(&recv_mssg);
                    // Check quit conditions
                    int commandType = strtol(recv_mssg.type, NULL, 10);
                    if(commandType == 4){ // QUIT
                        // Clean data field before rewrite
                        cleanMessage(&sent_mssg);
                        // Configure message content
                        sent_mssg.type[0] = '5';
                        int quittingcid = strtol(recv_mssg.data, NULL, 10);
                        sprintf(sent_mssg.data,"Quit request of client %d acknowledged.\n",quittingcid);
                        // Send output to client
                        scpipe = open(scPipePath,O_WRONLY);
                        if(scpipe == -1){
                            printf("sc pipe failed to be opened\n");
                            exit(-1);
                        }
                        write(scpipe,&sent_mssg,messageBufferLength);
                        close(scpipe);
                        // Print status
                        printStatus(&sent_mssg);
                    }
                    else if(commandType == 6){ // QUITALL
                        quitALLFlag = 1;
                        int quitAllcid = strtol(recv_mssg.data, NULL, 10);
                        printf("QUITALL request of client %d acknowledged.\n", quitAllcid);
                    }
                    else{
                        if(commandType == 9){ //COMPOUND COMMAND
                            // Create the unnamed pipe
                            int unnamedPipe [2];
                            if(pipe(unnamedPipe) < 0){
                                printf("Could not create unnamed pipe\n");
                                exit(-1);
                            }

                            // Divide compound command and pass into buffers
                            char* compoundCommand1 = strtok(recv_mssg.data,"|");
                            char* compoundCommand2 = strtok(NULL, "|");

                            //printf("compComm1: !%s!\n", compoundCommand1);
                            //printf("compComm2: !%s!\n", compoundCommand2);
                            csid = fork();
                            if(csid == -1){
                                printf("Runner child1 couldn't be created\n");
                                exit(-1);
                            }
                            else if(csid == 0){
                                //! ********* RUNNER CHILD1 CODE *********
                                // Runner child 1 will write to unnamed pipe
                                close(unnamedPipe[READEND]);
                                // Change output target
                                if(dup2(unnamedPipe[WRITEEND], fileno(stdout)) == -1){
                                    printf("Output target of runner child 1 couldn't be changed\n");
                                    exit(-1);
                                }
                                // close the pipe as we have it redirected to stdout
                                close(unnamedPipe[WRITEEND]);
                                // Extract command and arguments from data field and execute
                                char* command = strtok(compoundCommand1," ");
                                char* argument = strtok(NULL," ");
                                // At most 2 arguments will be passed including the command, no need for array of pointers etc.
                                char commandBuffer[256]; // MAXCOMLINE is 256
                                sprintf(commandBuffer, "/bin/%s",command);
                                execl(commandBuffer,command, argument, (char *)NULL);
                                // EXECLP DOES NOT RETURN, THIS IS UNREACHABLE CODE IF EVERYTHING WORKS OUT
                                perror("The error message is: ");
                                exit(-1);
                            }
                            else{
                                //! ********* SERVER CHILD CODE *********
                                int runner1id = wait(NULL);
                                printf("Runner child 1 %d terminated\n",runner1id);
                                // Create the output file
                                char outName[10];
                                sprintf(outName, "out%d",cid);
                                FILE * fp = fopen(outName,"w");
                                if(fp == NULL){
                                    printf("Output file couldn't be opened\n");
                                    perror("Error message is: ");
                                }
                                // Create input file
                                char inName[10];
                                sprintf(inName, "in%d",cid);
                                FILE * infp = fopen(inName,"w+");
                                if(infp == NULL){
                                    printf("Input file couldn't be opened\n");
                                    perror("Error message is: ");
                                }
                                csid = fork();
                                if(csid == -1){
                                    printf("Runner child2 couldn't be created\n");
                                    exit(-1);
                                }
                                else if(csid == 0){
                                    //! ********* RUNNER CHILD2 CODE *********
                                    // Runner child 2 will read the unnamed pipe
                                    close(unnamedPipe[WRITEEND]);
                                    // Change output destination to input file
                                    if(dup2(fileno(infp), fileno(stdout)) == -1){
                                        printf("Output (input) target of runner child 2 couldn't be changed\n");
                                        exit(-1);
                                    }
                                    // Read from unnamed pipe into the buffer
                                    char unnamedReadBuffer[PIPE_BUF];
                                    while(read(unnamedPipe[READEND],unnamedReadBuffer,PIPE_BUF) > 0 ){
                                        // Write input from pipe to input file
                                        printf("%s",unnamedReadBuffer);
                                    }
                                    // Close unnamed pipe after reading
                                    close(unnamedPipe[READEND]);
                                    // Change input source to input file
                                    freopen(inName,"r",stdin);
                                    fclose(infp);
                                    // Change output destination to output file
                                    if(dup2(fileno(fp), fileno(stdout)) == -1){
                                        printf("Output target of runner child 2 couldn't be changed\n");
                                        exit(-1);
                                    }
                                    fclose(fp);
                                    // execute the second part of the compound command
                                    // Extract command and arguments from data field and execute
                                    char* command = strtok(compoundCommand2," ");
                                    char* argument = strtok(NULL," ");
                                    // At most 2 arguments will be passed including the command, no need for array of pointers etc.
                                    char commandBuffer[256]; // MAXCOMLINE is 256
                                    sprintf(commandBuffer, "/bin/%s",command);
                                    execl(commandBuffer,command, argument, (char *)NULL);
                                    // EXECLP DOES NOT RETURN, THIS IS UNREACHABLE CODE IF EVERYTHING WORKS OUT
                                    perror("The error message is: ");
                                    exit(-1);

                                }
                                else{
                                    //! ********* SERVER CHILD CODE *********
                                    // Close the unnamed pipe for the server child
                                    close(unnamedPipe[READEND]);
                                    close(unnamedPipe[WRITEEND]);
                                    // wait for the termination of runner child 2
                                    int runner2id = wait(NULL);
                                    printf("Runner child 2 %d terminated\n",runner2id);

                                    // Open output file
                                    fp = fopen(outName,"r");
                                    if(fp == NULL){
                                        printf("Output file couldn't be opened\n");
                                        perror("Error is: ");
                                    }
                                    // Clean data field before rewrite
                                    cleanMessage(&sent_mssg);
                                    // Read output from file and set message type
                                    char sentDataBuffer[20];
                                    while(fgets(sentDataBuffer,20, fp) != NULL){
                                        strcat(sent_mssg.data, sentDataBuffer);
                                    }
                                    sent_mssg.type[0] = '8';
                                    // Close and delete the output file
                                    fclose(fp);
                                    // Delete the output file
                                    if(remove(outName) != 0){
                                        printf("Output file couldn't be deleted\n");
                                    }
                                    if(remove(inName) != 0){
                                        printf("Input file couldn't be deleted\n");
                                    }
                                    // Send output to client
                                    scpipe = open(scPipePath,O_WRONLY);
                                    if(scpipe == -1){
                                        printf("sc pipe failed to be opened\n");
                                        exit(-1);
                                    }
                                    write(scpipe,&sent_mssg,messageBufferLength);
                                    close(scpipe);
                                    // Print status
                                    printStatus(&sent_mssg);
                                }
                            }
                        }
                        else{ // NORMAL COMMAND
                            // Create output file
                            char outName[10];
                            sprintf(outName, "out%d",cid);
                            FILE * fp = fopen(outName,"w");

                            // Create a runner child
                            csid = fork();
                            //printf("id: %d\n",csid);
                            if(csid == -1){
                                printf("Runner child couldn't be created\n");
                                exit(-1);
                            }
                            else if(csid == 0){
                                //! ********* RUNNER CHILD CODE *********
                                // Change output target
                                if(dup2(fileno(fp), fileno(stdout)) == -1){
                                    printf("Output target couldn't be changed\n");
                                    exit(-1);
                                }
                                fclose(fp);
                                // Extract command and arguments from data field and execute
                                char* command = strtok(recv_mssg.data," ");
                                char* argument = strtok(NULL," ");
                                // At most 2 arguments will be passed including the command, no need for array of pointers etc.
                                char commandBuffer[256]; // MAXCOMLINE is 256
                                sprintf(commandBuffer, "/bin/%s",command);
                                execl(commandBuffer,command, argument, (char *)NULL);
                                // EXECLP DOES NOT RETURN, THIS IS UNREACHABLE CODE IF EVERYTHING WORKS OUT
                                perror("The error message is: ");
                                exit(-1);
                            }
                            else{
                                //! ********* SERVER CHILD CODE *********
                                printf("Waiting for runner child %d\n",csid);
                                int runnerid = wait(NULL);
                                printf("Runner child %d died\n", runnerid);
                                // Open output file
                                fp = fopen(outName,"r");
                                if(fp == NULL){
                                    printf("Output file couldn't be opened\n");
                                    perror("Error is: ");
                                }
                                // Clean data field before rewrite
                                cleanMessage(&sent_mssg);
                                // Read output from file and set message type
                                char sentDataBuffer[20];
                                while(fgets(sentDataBuffer,20, fp) != NULL){
                                    strcat(sent_mssg.data, sentDataBuffer);
                                }
                                sent_mssg.type[0] = '3';
                                // Close and delete the output file
                                fclose(fp);
                                // Delete the output file
                                if(remove(outName) != 0){
                                    printf("Output file couldn't be deleted\n");
                                }
                                // Send output to client
                                scpipe = open(scPipePath,O_WRONLY);
                                if(scpipe == -1){
                                    printf("sc pipe failed to be opened\n");
                                    exit(-1);
                                }
                                write(scpipe,&sent_mssg,messageBufferLength);
                                close(scpipe);
                                // Print status
                                printStatus(&sent_mssg);
                            }
                        }
                    }
                }while(!quitALLFlag);

                //exit(0);
            }
            else{
                //! ********* SERVER CODE *********
                clientCounter++;
            }
        }

    }
    free (bufferp);
    mq_close(mq);
    return 0;
}