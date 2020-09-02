#include "rtp.h"

int verbose = 0;
float error = 0;

void setVerboseLevel(int level) {
    verbose = level;
}

void setErrorLevel(float level) {
    error = level;
}

void p(char *format, ...) {
    va_list args;
    va_start(args, format);

    if (verbose) {
        printf(format, args);
    }

    va_end(args);
}

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

int checksumFrame(rtp_h *frame) {
    float r = ((float) rand() / (RAND_MAX));
    
    // Random error based on error rate
    if (r < error) {
        // Dont't send the packet
        p("LOST PACKET SEQ = %d, data = %s\n", frame->seq, frame->data);
        return rand();
    }

    int sum = 0;
    sum += frame->stime;
    sum += frame->flags;
    sum += frame->id;
    sum += frame->ack;
    sum += frame->seq;
    sum += frame->windowsize;

    int len = strlen(frame->data);
    for(int i = 0; i < len; i++) {
        sum += frame->data[i];
    }

    return sum;
}

int checkChecksum(char *data, int crc) {
    int chk;
    chk = checksum(data);

    return chk == crc ? 1 : 0;
}

int checkChecksumFrame(rtp_h *frame, int crc) {
    int chk;
    chk = checksumFrame(frame);

    return chk == crc ? 1 : 0;
}

void makePacket(rtp_h *frame, int seq, int ack, int flag, char* data) {
    frame->seq = seq;
    frame->ack = ack;
    
    struct timeval time;
	gettimeofday(&time, NULL);
	frame->stime = (long)time.tv_sec;
	frame->stime *= 1000000;
	frame->stime += time.tv_usec;


    if (flag != 0) {
        frame->flags = flag;
        memset(frame->data, 0, DATA_SIZE);
    } else {
        frame->flags = 0;
        memset(frame->data, 0, DATA_SIZE);
        memcpy(frame->data, data, strlen(data));
    }

    frame->crc = 0;
    frame->crc = checksumFrame(frame);
}

int getData(TransmissionInfo *ti, rtp_h *frame, int timeout) {
    socklen_t len = sizeof(&ti->dest);
    int res = recvfrom(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, &len);

    if (checkChecksumFrame(frame, frame->crc)) {
        return res;
    } else {
        printf("Checksum failed, SEQ = %d, CRC = %d\n", frame->seq, frame->crc);
        return -1;
    }

}

int sendData(TransmissionInfo *ti, rtp_h *frame) {
    int res = randomSeq();
    struct timeval time;
    socklen_t peer_addr_len;
	peer_addr_len = sizeof(struct sockaddr_storage);

    // Set time on our packet in microseconds
    // Used for timeout thread
	gettimeofday(&time, NULL);
	frame->time = (long)time.tv_sec;
	frame->time *= 1000000;
	frame->time += time.tv_usec;
    
    float r = ((float) rand() / (RAND_MAX));
    
    // Random error based on error rate
    if (r > error) {
        // Send the packet.
        res = sendto(ti->socket, frame, FRAME_SIZE, 0, (struct sockaddr *)&ti->dest, peer_addr_len);
    } else {
        // Dont't send the packet
        p("LOST PACKET SEQ = %d, data = %s\n", frame->seq, frame->data);
    }

    return res;
}

// Not used anymore
int sendLostData(TransmissionInfo *ti, rtp_h *frame) {
    p("LOST PACKET SEQ = %d, data = %s\n", frame->seq, frame->data);
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

void initQueue(queue* q, int len, enum QueueType type) {
    q->count = 0;
    q->size = len;
    q->queue = malloc(len * sizeof(rtp_h));
    q->type = type;
}

// Put a packet in a queue
int enqueue(TransmissionInfo *transmissionInfo, queue *q, rtp_h frame, enum QueueType type) {

    if (isQueueFull(q)) {
        p("Queue is full");
        return -1;
    }

    // if (frame.seq == 0) {
    //     p("Error adding, SEQ == 0");
    //     exit(EXIT_FAILURE);
    // }

    int index = q->count;

    switch (type) {
        case SENT:
            memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
            q->count++;
            transmissionInfo->s_vars.next++;
            printQueue(q);
            break;

        case RECEIVED:
            // If the packet is already in the queue, dont add it.
            for (int i = 0; i < q->count; i++) {
                if (q->queue[i].seq == frame.seq) {
                    p("Already in queue.\n");
                    return -1;
                }
            }
            // If the packet is applicable and passes the checksum, put it in the queue
            if (frame.seq >= transmissionInfo->r_vars.next && frame.seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->r_vars.next)){
                if (checkChecksumFrame(&frame, frame.crc)) {
                    memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
                    q->count++;
                } else {
                    p("Checksum failed, SEQ = %d, CRC = %d\n", frame.seq, frame.crc);
                    return -1;
                }

            // Outside window size.
            } else {
                p("Received packet outside window size, packet not queued. SEQ = %d\n", frame.seq);
                return -1;
            }
            break;

        case ACKNOWLEDGEMENT:
            transmissionInfo->r_vars.next++;
            if (frame.ack >= transmissionInfo->s_vars.oldest && frame.ack < (transmissionInfo->s_vars.window_size) + (transmissionInfo->s_vars.oldest)){
                memcpy((&q->queue[index]), &frame, sizeof(rtp_h));
                q->count++;
            } else {
                p("Received ACK outside window size. SEQ = %d\n", frame.ack);
            }
            break;    

        default:
            break;
    }

    return 1;
}

void printQueue(queue *q) {
    if (q->type == ACKNOWLEDGEMENT)
        p("\nACK QUEUE\n");
    else if (q->type == SENT)
        p("\nSENT QUEUE\n");
    else if (q->type == RECEIVED)
        p("\nREAD QUEUE\n");

    for (int i = 0; i < q->size; i++) {
        if (q->type == ACKNOWLEDGEMENT)
            p("%d: SEQ = %d, ACK = %d\n", i, q->queue[i].seq, q->queue[i].ack);
        else
            p("%d: SEQ = %d, DATA = %s\n", i, q->queue[i].seq, q->queue[i].data);

    }
    p("\n");
}

void dequeue(queue *q) {

    if (isQueueEmpty(q)) {
        p("Queue is empty\n");
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
        p("Queue is empty\n");
        return;
    }
    
    memmove(&q->queue[index], &q->queue[index + 1], ((q->size - 1) - index) * sizeof(rtp_h));

    memset(&q->queue[q->size - 1], 0, sizeof(rtp_h));

    q->count--;

}

void clearQueue(queue *q) {
    memset(q->queue, 0, sizeof(rtp_h) * q->size);
    q->count = 0;
}

int isInQueue(queue* q, int seq) {
    for (int i = 0; i < q->count; i++) {
        switch (q->type) {
            case ACKNOWLEDGEMENT:
                if (q->queue[i].ack == seq)
                    return i;
                break;
            default:
                if (q->queue[i].seq == seq)
                    return i;
                break;
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

    timeout.tv_usec = 1000000;

    while (1) {
            
        // Get time
        gettimeofday(&currentTime, NULL);
        // Get time in microseconds.
        long mTime = currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;

        for (int i = 0; i < transmissionInfo->s_vars.window_size; i++) {
            // If our queue item is in our window, check if it needs resend
            if (q->queue[i].seq >= transmissionInfo->s_vars.oldest && q->queue[i].seq < (transmissionInfo->s_vars.window_size) + (transmissionInfo->s_vars.oldest)){
                // If timeout is exceeded, resend the packet
                // Note that the resend is still prone to errors.
                if (timeout.tv_usec + q->queue[i].time <= mTime) {
                    rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

                    makePacket(frame, q->queue[i].seq, 0, q->queue[i].flags, q->queue[i].data);
                    sendData(transmissionInfo, frame);
                    q->queue[i].time = mTime;
                    p("TIMEOUT: Resending packet, SEQ = %d, data = %s\n", frame->seq, frame->data);
                    free(frame);
                }
            }
        }

         usleep(100000);
    }
}

void *selectiveTimeout(void *args) {
    // Get args (rtp.h timeout_args)
    struct timeout_arguments *t_args = args;
    TransmissionInfo *transmissionInfo = t_args->arg1;
    queue *q = t_args->arg2;

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
                // printQueue(q);
                p("Checking timeout of packet SEQ = %d\n", q->queue[i].seq);
                // If timeout is exceeded
                if (timeout.tv_usec + q->queue[i].time <= mTime) {
                    // if(isInQueue(ackQueue, q->queue[i].seq) == -1)
                    // {
                        rtp_h *frame = (rtp_h*)malloc(FRAME_SIZE);

                        makePacket(frame, q->queue[i].seq, 0, q->queue[i].flags, q->queue[i].data);
                        sendData(transmissionInfo, frame);
                        // Reset the time.
                        q->queue[i].time = mTime;
                        p("TIMEOUT: Resending packet, SEQ = %d, data = %s\n", frame->seq, frame->data);
                    // } else {
                    //     p("sentQueue packet SEQ = %d not in ack queue, no timeout send\n", q->queue[i].seq);
                    // }
                }
            }
        }

        usleep(1000000);

    }
    
}

void pTimestamp() {
    char log_buff[30];
    int time_len = 0;
    struct tm *tm_info;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    time_len+=strftime(log_buff, sizeof log_buff, "%H:%M:%S", tm_info);
    time_len+=snprintf(log_buff+time_len,sizeof log_buff-time_len,".%03ld: ",tv.tv_usec/1000);

    printf(log_buff);
    return;
}