#ifndef _SERVER_H
#define _SERVER_H

typedef struct server {
  char myIp[30];
  char myPort[10];
  int key;

  char* nextIp;
  char* previousIp;
  char* doubleNextIp;
}Server;


void createServer(Server*);


#endif