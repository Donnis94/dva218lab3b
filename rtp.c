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

void makePacket(rtp_h *frame, int seqNr, int flag, char* data) {
    frame->seq = seqNr;
    
    frame->crc = 0;


    if (flag != 0) {
        frame->flags = flag;
        memset(frame->data, 0, DATA_SIZE);
    } else {
        frame->flags = 0;
        memset(frame->data, 0, DATA_SIZE);
        memcpy(frame->data, data, strlen(data));
    }
}

int getData(TransmissionInfo *ti, rtp_h *frame) {
    socklen_t len = sizeof(&ti->dest);
    return recvfrom(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, &len);
}

int sendData(TransmissionInfo *ti, rtp_h *frame) {
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