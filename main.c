#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checkIpPortValid(int, char **);
int valid_digit(char *);
int checkIp(char *);


int main(int argc, char *argv[]) {
    if(!checkIpPortValid(argc, argv)) exit(0); // Check if the dkt program was summoned with 3 arguments
    char ip[15];
    strcpy(ip, "194.210.157.160");
    int port;
    port = (int) *argv[2];

    while(1) { // The code will run until the "exit" command is summoned
        char buffer[100];
        char lixo[100];
        // Show the user interface
        printf("Available commands:\n\n new i - create a new ring with the i server only \n exit\n");
        fgets(buffer, 100 , stdin);
        const char delim[2] = " ";
        if(strcmp(strtok(strdup(buffer), delim), "new") == 0) { // Check if the command is the new command
            int server;
            sscanf(buffer, "%s %d", lixo, &server);
            printf("Server: %d\n\n", server);
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
