#ifndef _SERVER_H
#define _SERVER_H

typedef struct server {
  char* myIp;
  char* myPort;
  int key;

  char* nextIp;
  char* previousIp;
  char* doubleNextIp;
}Server;


void createServer(char*);


#endif