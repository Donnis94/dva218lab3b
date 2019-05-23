#define PORT 5555
#define DST "127.0.0.1"

#define FRAME_SIZE 544
#define DATA_SIZE 512

#define INIT 0
#define WAIT_SYN 1
#define WAIT_SYNACK 2
#define WAIT_ACK 3
#define WAIT_FINACK 4
#define CLOSED 99

#define ESTABLISHED 1337

#define SYN 420
#define ACK 421
#define FIN 422

typedef struct {
    int flags;
    int id;
    int seq;
    int windowsize;
    int crc;
    char data[DATA_SIZE];
} rtp_h;

typedef struct
{
	rtp_h *queue;
	int size;
	int count;
} queue;

typedef struct {
    int seq;
} Variables;

typedef struct {
    struct sockaddr_in host;
    struct sockaddr_in dest;
    Variables s_vars;
    Variables r_vars;
    int socket;
} TransmissionInfo;

void makePacket(rtp_h *frame, int seqNr, int flag, char* data);
int getData(TransmissionInfo *ti, rtp_h *frame);
int sendData(TransmissionInfo *ti, rtp_h *frame);
void initState();
void teardown();