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

void incrementSeq(TransmissionInfo *transmissionInfo) {
    transmissionInfo->s_vars.seq = transmissionInfo->s_vars.seq + 1;
}

int getSeq(TransmissionInfo *transmissionInfo) {
    return transmissionInfo->s_vars.seq;
}

void initQueue(queue* q, int len) {
    q->queue = (rtp_h*)malloc(len * sizeof(rtp_h));
    q->size = len;
    q->count = 0;
}