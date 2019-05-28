#include "rtp.h"

int randomSeq() {
    return rand() % 1000;
}

int checksum(char *data) {
    int sum = 0;
    int len = strlen(data);
    for(int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

int checkChecksum(char *data, int crc) {
    int chk;
    chk = checksum(data);

    return chk == crc ? 1 : 0;
}

void makePacket(rtp_h *frame, int seq, int ack, int flag, char* data) {
    frame->seq = seq;
    frame->ack = ack;
    


    if (flag != 0) {
        frame->flags = flag;
        memset(frame->data, 0, DATA_SIZE);
    } else {
        frame->flags = 0;
        frame->crc = checksum(data);
        // memset(frame->data, 0, DATA_SIZE);
        memcpy(frame->data, data, strlen(data));
    }
}

int getData(TransmissionInfo *ti, rtp_h *frame, int timeout) {
    socklen_t len = sizeof(&ti->dest);
    return recvfrom(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, &len);
}

int sendData(TransmissionInfo *ti, rtp_h *frame) {
    int res;
    struct timeval time;
    socklen_t peer_addr_len;
	peer_addr_len = sizeof(struct sockaddr_storage);

    // Set time on our packet in microseconds
    // Used for timeout thread
	gettimeofday(&time, NULL);
	frame->time = (long)time.tv_sec;
	frame->time *= 1000000;
	frame->time += time.tv_usec;
    
    res = sendto(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, peer_addr_len);
    return res;
}

int sendLostData(TransmissionInfo *ti, rtp_h *frame) {
    int res = randomSeq();
    struct timeval time;

    // Set time on our packet in microseconds
    // Used for timeout thread
	gettimeofday(&time, NULL);
	frame->time = (long)time.tv_sec;
	frame->time *= 1000000;
	frame->time += time.tv_usec;
    
    return res;
}

void initQueue(queue* q, int len) {
    q->count = 0;
    q->size = len;
    q->queue = malloc(len * sizeof(rtp_h));
}

int enqueue(TransmissionInfo *transmissionInfo, queue *q, rtp_h frame, enum QueueType type) {

    if (isQueueFull(q)) {
        printf("Queue is full");
        return -1;
    }

    if (frame.seq == 0) {
        printf("Error adding, SEQ == 0");
        exit(EXIT_FAILURE);
    }

    int index = q->count;

    switch (type) {
        case SENT:
            memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
            q->count++;
            transmissionInfo->s_vars.next++;
            break;

        case RECEIVED:
            for (int i = 0; i < q->count; i++) {
                if (q->queue[i].seq == frame.seq) {
                    printf("Already in queue.\n");
                    return -1;
                }
            }
            if (frame.seq >= transmissionInfo->r_vars.next && frame.seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->r_vars.next)){
                if (checkChecksum(frame.data, frame.crc)) {
                    memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
                    q->count++;
                } else {
                    printf("Checksum failed, SEQ = %d, CRC = %d\n", frame.seq, frame.crc);
                    return -1;
                }

            } else {
                printf("Received packet outside window size. SEQ = %d", frame.seq);
            }
            break;

        case ACKNOWLEDGEMENT:
            if (frame.seq >= transmissionInfo->s_vars.oldest && frame.seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->s_vars.oldest)){
                memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
                q->count++;
            } else {
                printf("Received packet outside window size. SEQ = %d", frame.seq);
            }
            break;    

        default:
            break;
    }

    return 1;
}

void dequeue(queue *q) {

    if (isQueueEmpty(q)) {
        printf("Queue is empty\n");
        return;
    }

	if (q->size > 1) {
        memmove(&q->queue[0], &q->queue[1], (q->size - 1) * sizeof(rtp_h));
    }

    memset(&q->queue[q->size - 1], 0, sizeof(rtp_h));

    q->count--;

}

void removeFromQueue(queue *q, int index) {
    if (isQueueEmpty(q)) {
        printf("Queue is empty\n");
        return;
    }

	if (q->size > 1) {
        memmove(&q->queue[index], &q->queue[1], (q->size - 1) * sizeof(rtp_h));
    }

    memset(&q->queue[q->size - 1], 0, sizeof(rtp_h));

    q->count--;
}

void clearQueue(queue *q) {
    memset(q->queue, 0, sizeof(rtp_h) * q->size);
    q->count = 0;
}

int isInQueue(queue* q, int seq) {
    for (int i = 0; i < q->count; i++) {
        if (q->queue[i].seq == seq) {
            return i;
        }
    }

    return -1;
}

int isQueueFull(queue *q) {
    return q->count == q->size ? 1 : 0;
}

int isQueueEmpty(queue *q) {
    return q->count == 0 ? 1 : 0;
}

void *timeout(void *args) {
    // Get args (rtp.h timeout_args)
    struct timeout_arguments *t_args = args;
    TransmissionInfo *transmissionInfo = t_args->arg1;
    queue *q = t_args->arg2;

    struct timeval timeout;
    struct timeval currentTime;

    timeout.tv_usec = 100000;

    while (1) {
            
        // Get time
        gettimeofday(&currentTime, NULL);
        // Get time in microseconds.
        long mTime = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;

        for (int i = 0; i < transmissionInfo->s_vars.window_size; i++) {
            // If our queue item is in our window, check if it needs resend
            if (q->queue[i].seq >= transmissionInfo->s_vars.oldest && q->queue[i].seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->s_vars.oldest)){
                // If timeout is exceeded
                if (timeout.tv_usec + q->queue[i].time <= mTime) {
                    makePacket(&q->queue[i], q->queue[i].seq, 0, 0, q->queue[i].data);
                    sendData(transmissionInfo, &q->queue[i]);
                    printf("TIMEOUT: Resending packet, SEQ = %d, data = %s\n", q->queue[i].seq, q->queue[i].data);
                }
            }
        }

         usleep(10000);
    }
}
void *selectiveTimeout(void *args) {
// Get args (rtp.h timeout_args)
struct timeout_arguments *t_args = args;
TransmissionInfo *transmissionInfo = t_args->arg1;
queue *q = t_args->arg2;
queue *p = t_args->arg3;

struct timeval timeout;
struct timeval currentTime;

timeout.tv_usec = 1000000;

while (1) {
        
    // Get time
    gettimeofday(&currentTime, NULL);
    // Get time in microseconds.
    long mTime = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;

    for (int i = 0; i < transmissionInfo->s_vars.window_size; i++) {
        // If our queue item is in our window, check if it needs resend
        if (q->queue[i].seq >= transmissionInfo->s_vars.oldest && q->queue[i].seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->s_vars.oldest)){
            // If timeout is exceeded
            if (timeout.tv_usec + q->queue[i].time <= mTime) {
                for (int e = 0; e < WINDOW_SIZE; e++) {
                    if (q->queue[i].seq == p->queue[e].seq) {
                        makePacket(&q->queue[i], q->queue[i].seq, 0, 0, q->queue[i].data);
                        sendData(transmissionInfo, &q->queue[i]);
                        printf("TIMEOUT: Resending packet, SEQ = %d, data = %s\n", q->queue[i].seq, q->queue[i].data);
                    }
                }
            }
        }
    }

    usleep(1000000);

    }
    
}