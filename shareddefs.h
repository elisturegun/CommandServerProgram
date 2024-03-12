#include <sys/wait.h>
#include <stdio.h>
#include <string.h>


#define READEND 0
#define WRITEEND 1
#define PIPE_BUF 4096
#define MAX_CLIENT_NUM 5

struct item {
	int id;
	char astr[64];
};

struct message{
    char size[4];
    char type[1];
    char padding[3];
    char data[PIPE_BUF];
};

void cleanMessage(struct message* message){
    memset(message->data, 0 , sizeof message->data);
    memset(message->type, 0, sizeof message->type);
    memset(message->size, 0, sizeof message->size);
    memset(message->padding, 0, sizeof message->padding);
    return;
}

struct mqmessage{
    char size[4];
    char type[1];
    char padding[3];
    char data[5];
};

int mqReadWsize (struct mqmessage* mqmessage){
    char size[10];
    //this is terrible code, but I am too pissed that there isn't a built-in function for this
    size[3] = mqmessage->size[0];
    size[2] = mqmessage->size[1];
    size[1] = mqmessage->size[2];
    size[0] = mqmessage->size[3];
    return strtol(size,NULL,10);
}
void mqWriteWsize (struct mqmessage* mqmessage, int wsize){
    char size[10];
    sprintf(size, "%d",wsize);
    //this is terrible code, but I am too pissed that there isn't a built-in function for this
    mqmessage->size[0] = size[3];
    mqmessage->size[1] = size[2];
    mqmessage->size[2] = size[1];
    mqmessage->size[3] = size[0];
}

int mssgReadWsize (struct message* message){
    char size[10];
    //this is terrible code, but I am too pissed that there isn't a built-in function for this
    size[3] = message->size[0];
    size[2] = message->size[1];
    size[1] = message->size[2];
    size[0] = message->size[3];
    return strtol(size,NULL,10);
}
void mssgWriteWsize (struct message* message, int wsize){
    char size[10];
    sprintf(size, "%d",wsize);
    //this is terrible code, but I am too pissed that there isn't a built-in function for this
    message->size[0] = size[3];
    message->size[1] = size[2];
    message->size[2] = size[1];
    message->size[3] = size[0];
}


// Create two named pipes
int cspipe;
int scpipe;

