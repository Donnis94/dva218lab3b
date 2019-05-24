#include "rtp.h"

int randomSeq() {
    return rand();
}

void makePacket(rtp_h *frame, int seq, int ack, int flag, char* data) {
    frame->seq = seq;
    frame->ack = ack;
    
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

int getData(TransmissionInfo *ti, rtp_h *frame, int timeout) {
    fd_set set;
	FD_ZERO(&set);
	FD_SET(ti->socket, &set);
    struct timeval Timeout;
    Timeout.tv_sec = 0;
	Timeout.tv_usec = timeout; 

    socklen_t len = sizeof(&ti->dest);
    if(select(FD_SETSIZE, &set, 0, 0, &Timeout) != 0) {
        return recvfrom(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, &len);
    } else {
        return -1;
    }
}

int sendData(TransmissionInfo *ti, rtp_h *frame) {
    int res;
    socklen_t peer_addr_len;
	peer_addr_len = sizeof(struct sockaddr_storage);
    
    res = sendto(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, peer_addr_len);
    return res;
}

// void incrementSeq(TransmissionInfo *transmissionInfo) {
//     transmissionInfo->s_vars.seq = transmissionInfo->s_vars.seq + 1;
// }

// int getSeq(TransmissionInfo *transmissionInfo) {
//     return transmissionInfo->s_vars.seq;
// }

void initQueue(queue* q, int len) {
    q->queue = calloc(len, sizeof(rtp_h));
    q->size = len;
    q->count = 0;
}

void enqueue(TransmissionInfo *transmissionInfo, queue *q, rtp_h *frame, enum QueueType type) {

    if (frame->seq == 0) {
        printf("Error adding, SEQ == 0");
        exit(EXIT_FAILURE);
    }

    memcpy(&q->queue[q->count * sizeof(rtp_h)], frame, sizeof(rtp_h));
    q->count++;

    switch (type) {
        case SENT:
            transmissionInfo->s_vars.next++;
            break;

        case RECEIVED:
            // Check window size
            break;

        case ACKNOWLEDGEMENT:
            // Check window size
            break;    

        default:
            break;
    }
}

rtp_h* dequeue(queue *q) {
    rtp_h *frame = &q->queue[0];
	if (q->size > 1) {
        memmove(&q->queue[0], &q->queue[1], (q->size - 1) * sizeof(rtp_h));
    }

    memset(&q->queue[q->size - 1], 0, sizeof(rtp_h));

    q->count--;
    return frame;
}

int isQueueFull(queue *q) {
    return q->count == q->size ? 1 : 0;
}

int isQueueEmpty(queue *q) {
    return q->count == 0 ? 1 : 0;
}