/*
* RCI Project - Distributed Hashtable - João Maria Janeiro and Guilherme Viegas
* Key values ​​equal to -1 mean that it does not contain information about that server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <math.h>
#include <sys/select.h>
#include <errno.h>

int checkIpPortValid(int, char **);
int valid_digit(char *);
int checkIp(char *);

struct timeval tv;
char aux[100];


int main(int argc, char *argv[]) {
    tv.tv_sec = 2;        // 2Secs Timeout
    tv.tv_usec = 0;        // Safer to initialize

    if(!checkIpPortValid(argc, argv)) exit(0); // Check if the dkt program was summoned with 3 arguments
    Server * myServer = (Server*)malloc(sizeof(Server));
    strcpy(myServer->myIp, "127.0.0.1");
    strcpy(myServer->myPort, argv[2]);
      

    while(1) { // The code will run until the "exit" command is summoned
        char buffer[100];
        // Show the user interface
        printf("\n\nAvailable commands:\n\n new i \n sentry i succi succi.IP succi.TCP \n exit\n");
        fgets(buffer, 100 , stdin);
        const char delim[2] = " ";
        if(strcmp(strtok(strdup(buffer), delim), "new") == 0) { // If its the "new" command then create a ring with that server
            sscanf(buffer, "%s %d", aux, &(myServer->myKey)); // Get the server key
            createServer(myServer);
        } else  if(strcmp(strtok(strdup(buffer), delim), "sentry") == 0) { // If its the sentry command open a connection with nextServer 
            sscanf(buffer, "%s %d %d %s %s", aux, &(myServer->myKey), &(myServer->nextKey), myServer->nextIp, myServer->nextPort); // Get the successor details
            
            printf("Trying to enter\n");
            myServer->nextConnFD = connectToNextServer(myServer); // Set the next server as the given server and establish a connection [MISSING CONFIRMATION IF EXISTS]
            
            sprintf(buffer, "NEW %d %s %s\n", myServer->myKey, myServer->myIp, myServer->myPort);
            int n = write(myServer->nextConnFD, buffer, strlen(buffer)); // Give the successor your details
            if(n == -1)/*error*/exit(1);
                
            // Get the info about the sucessor of the successor
            n = read(myServer->nextConnFD, buffer, 128);
            if(n==-1)/*error*/exit(1);
            write(1,"\nReceived: ",10); write(1,buffer,n);
            
            sscanf(buffer, "%s %d %s %s", aux, &(myServer->doubleNextKey), myServer->doubleNextIp, myServer->doubleNextPort); // Get the double successor details and update

            //setsockopt(myServer->nextConnFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)); //2 second timeout on the reads

            createServer(myServer); //Now that the entry connections are established and stable it's time enter in listening mode [AQUI VAI FALTAR MANTER GUARDADAS AS CONEXÕES QUE FORAM ESTABELECIDADES ANTES(createServer() talvez deva ser alterado)]
        } else if(strcmp(buffer, "exit\n") == 0) {
            exit(0);
        }
    }

    return 0;
}

int checkIpPortValid(int _argc, char *_argv[]) {
    if(_argc !=  3) {
        printf("Number of Arguments must be 3\n");
        exit(0);
    }
    if(!checkIp(_argv[1])) {
        exit(0);
    }
    return 1;
}

/* return 1 if string contain only digits, else return 0 */
int valid_digit(char *ip_str) { 
    while (*ip_str) { 
        if (*ip_str >= '0' && *ip_str <= '9') 
            ++ip_str; 
        else
            return 0; 
    } 
    return 1; 
}

/*Check the Ip Address format*/
int checkIp(char * ip) {
    char* ptr;
    int i, num, dots = 0;

    if(strlen(ip) != 15) {
        printf("Ip Size must be 15 chars\n");
        return 0;
    }

    ptr = strtok(ip, ".");

    if (ptr == NULL) 
        return 0; 
  
    while (ptr) { 
        if (!valid_digit(ptr)) {
            printf("Not Valid Digit\n");
            return 0; 
        }
  
        num = atoi(ptr); 
  
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); 
            if (ptr != NULL) 
                ++dots; 
        } else {
            printf("Strange Format my boy");
            return 0; 
        }
    } 
  
    /* valid IP string must contain 3 dots */
    if (dots != 3) {
        printf("Number of dots must be of 3 %d\n", dots);
        return 0; 
    }
    return 1; 
}
