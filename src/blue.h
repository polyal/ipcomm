#define CHUNK 32768
#define MAX_DEVS 255 // max devices returned during discovery
#define DISC_UNIT 8  // unit * 1.28sec time spent searching for devices
#define ADDR_SIZE 18 // size in chars of a bluetooth address
#define MAX_CON_DEVS 30 // number of connected devices with rfcomm
#define MAX_NAME_LEN 255

typedef struct _devInf {
	int devId;
    char addr[ADDR_SIZE];
    char name[MAX_NAME_LEN];
} devInf;

int findLocalDevices(devInf ** const devs, int * const numDevs);

int findDevices(devInf ** const devs, int * const numDevs);

int client(const char* const dest, int channel, const char* const src, int sc, const char* const data, int size);

int server(char addr[ADDR_SIZE], const char* const myAddr, int channel, char ** const data, int* const size);


int initServer(int * const err);

int connect2Server(const int channel, const char* const dest, int * const err);

int sendReqWait4Resp(const int sock, const char * const reqData, const int size, char recData[CHUNK], int * const respSize);

int listen4Req(int sock, char addr[ADDR_SIZE], int * const err);

int fetchRequestData(int sock, char ** const data, int * const size);

int sendResponse(int sock, const char * const data, const int size);

int closeSocket(int sock);
