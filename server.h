#ifndef _SERVER_H
#define _SERVER_H

typedef struct server {
  char myIp[30];
  char myPort[10];
  int key;

  char nextIp[30];
  char nextPort[10];
  int nextKey;

  char doubleNextIp[30];
  char doubleNextPort[10];
  int doubleNextKey;

  int prevConnFD; // Holds the TCP connection to previous server
  int nextConnFD; // Holds the TCP connection to next server
}Server;


void createServer(Server*);
int connectToNextServer(Server*);
void decodeMessage(char*, int*, char[16], char[10]);


#endif