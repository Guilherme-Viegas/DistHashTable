#ifndef _SERVER_H
#define _SERVER_H

typedef struct server {
  char myIp[30];
  char myPort[10];
  int myKey;

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
void serverIsEntering(char[128], int *, Server*);
void printServerData(Server*);
void copyDoubleToNext(Server *);
int distance(int, int);
int connectToGivenServer(char[30], char[10]);

#endif