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

int getData(TransimssionInfo *ti, rtp_h *frame) {
    return recvfrom(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, sizeof(ti->dest));
}

int sendData(TransimssionInfo *ti, rtp_h *frame) {
    int res;
    socklen_t peer_addr_len;
	  peer_addr_len = sizeof(struct sockaddr_storage);

    res = sendto(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, peer_addr_len);

    return res;
}

// void state(int sock) {
//     rtp_h event;
//     event = getData(sock);
//     printf("%d", event.flags);

//     while(1) {
//         switch (state)
//         {
//         case INIT:
//             if (event == send_data) {
//                 state = WAIT_ACK;
//                 send (DATA_TO_RECEIVER);
//             }
//             break;
        
//         case WAIT_SYN:
//             if (event == got_syn) {
//                 state = WAIT_ACK;
//                 send (SYNC_ACK_TO_SENDER);
//             }
//             break;

//         case WAIT_SYNACK:
//             break;

//         case WAIT_ACK:
//             break;

//         default:
//             if (state == WAIT_ACK)
//                 resend (DATA_TO_SERVER);
//             break;
//         }
//     }
// }