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

struct sockaddr_in addr;

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin_family = AF_INET;     
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin_port = htons(port);   
  /* Get info about host. */
  hostInfo = gethostbyname(hostName); 
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* 
   * Fill in the host name into the sockaddr_in struct.
   * h_addr -> h_addr_list[0] as defined in netdb.h -> struct hostent
   * 
   */
  name->sin_addr = *(struct in_addr *)hostInfo->h_addr_list[0];
}

rtp_h getData(int sock) {
    rtp_h *data = malloc(sizeof(rtp_h));
    struct sockaddr serv_addr;
    initSocketAddress(&addr, DST, PORT);
    struct socklen_t *size_len;
    
    int res = recvfrom(sock, data,sizeof(data),0,(struct sockaddr *)&serv_addr,&size_len);

    if (res > 0) {
        return *data;
    }

    exit(EXIT_FAILURE);
}

int sendData(int sock, rtp_h *data) {
    struct sockaddr *serv_addr;
    int res;

    res = sendto(sock,&data,sizeof(data),0,(struct sockaddr *)&addr,sizeof(addr));

    return res;
}

void state(int sock) {
    rtp_h event;
    event = getData(sock);
    printf("%d", event.flags);

    // while(1) {
    //     switch (state)
    //     {
    //     case INIT:
    //         if (event == send_data) {
    //             state = WAIT_ACK;
    //             send (DATA_TO_RECEIVER);
    //         }
    //         break;
        
    //     case WAIT_SYN:
    //         if (event == got_syn) {
    //             state = WAIT_ACK;
    //             send (SYNC_ACK_TO_SENDER);
    //         }
    //         break;

    //     case WAIT_SYNACK:
    //         break;

    //     case WAIT_ACK:
    //         break;

    //     default:
    //         if (state == WAIT_ACK)
    //             resend (DATA_TO_SERVER);
    //         break;
    //     }
    // }
}