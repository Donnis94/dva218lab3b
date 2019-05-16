/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 * 
 * Author Donatello Piancazzo dpo16001 and Oskar Berglund obd16004 Alexander Andersson Tholin atn17004 2019/05/02
 * 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include "rtp.h"

#define hostNameLength 50
#define messageLength 256

int makeSocket(TransimssionInfo *ti, char* hostName) {	
	struct hostent *hostInfo;
	hostInfo = gethostbyname(hostName);
	ti->dest.sin_family = AF_INET;
	ti->dest.sin_port = htons(PORT);
	ti->dest.sin_addr = *(struct in_addr *)hostInfo->h_addr_list[0];
	
	// Create socket
	ti->socket = socket(AF_INET, SOCK_DGRAM, 0);
	return 0;
}

int main (int argc, const char *argv[]){
    TransimssionInfo ti;
    char hostName[hostNameLength];

    /* Check arguments */
    if(argv[1] == NULL) {
        perror("Usage: client [host name]\n");
        exit(EXIT_FAILURE);
    }
    else {
        strncpy(hostName, argv[1], hostNameLength);
        hostName[hostNameLength - 1] = '\0';
    }

    makeSocket(&ti, hostName);


    return EXIT_SUCCESS;
}