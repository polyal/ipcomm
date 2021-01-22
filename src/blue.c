#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "blue.h"

#define DEBUG 1


int findLocalDevices(devInf ** const devs, int * const numDevs){
    struct hci_dev_list_req *devList = NULL;
    struct hci_dev_req *devReq = NULL;
    int status = -1;
    int i, sock;

    if (!devs || !numDevs){
        printf("Find Local Error: Invalid Input.\n");
        goto findLocalDevicesCleanup;
    }

    sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (sock < 0){
        printf("Find Local Error: Can't allocate socket.\n");
        goto findLocalDevicesCleanup;
    }

    devList = malloc(HCI_MAX_DEV * sizeof(*devReq) + sizeof(uint16_t));
    if (!devList) {
        status = errno;
        goto findLocalDevicesCleanup;
    }

    memset(devList, 0, HCI_MAX_DEV * sizeof(struct hci_dev_req) + sizeof(uint16_t));
    devList->dev_num = HCI_MAX_DEV;
    devReq = devList->dev_req;

    // request list of devices from microcontroller
    if (ioctl(sock, HCIGETDEVLIST, (void *) devList) < 0) {
        printf("Find Local Error: IOCTL.\n");
        status = errno;
        goto findLocalDevicesCleanup;
    }

    if (devList->dev_num < 1)
        goto findLocalDevicesCleanup;
printf("%d\n", devList->dev_num);
    *devs = malloc(sizeof(devInf)* devList->dev_num);
    if (*devs == NULL) 
        goto findLocalDevicesCleanup;

    bdaddr_t bdaddr;// = { 0 };
    char addr[ADDR_SIZE] = { 0 };
    for (i = 0; i < devList->dev_num; i++, devReq++) {
        if (hci_test_bit(HCI_UP, &devReq->dev_opt)){
            char name[248] = { 0 };
            hci_devba(devReq->dev_id, &bdaddr);
            ba2str(&bdaddr, addr);

            int sock2dev = hci_open_dev( devReq->dev_id );
            hci_read_local_name(sock2dev, 248, name, 0);
            if (sock2dev >= 0) close(sock2dev);

            (*devs)[i].devId = devReq->dev_id;
            memcpy((*devs)[i].addr, addr, strlen(addr));
            (*devs)[i].name[0] = '\0';
            printf("Find Local Dev: %d %s %s \n", (*devs)[i].devId, (*devs)[i].addr, name);
        }       
    }

    *numDevs = devList->dev_num;
    status = 0;

findLocalDevicesCleanup:
    if (devList) free(devList);
    if (sock >= 0) close(sock);

    return status;
}



int findDevices(devInf ** const devs, int * const numDevs){
    int status = -1;
    inquiry_info *ii = NULL;
    int num_rsp;
    int dev_id, sock;
    char addr[ADDR_SIZE] = { 0 };
    char name[248] = { 0 };

    *numDevs = 0;

    // get first available local bluetooth adapter
    dev_id = hci_get_route(NULL);

    // opens a socket to the bluetooth microcontroller with id dev_id
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        printf("Discovery Error: Failed to open socket.\n");
        goto findDevicesCleanup;
    }

    ii = (inquiry_info*)malloc(sizeof(inquiry_info)* MAX_DEVS);
    if (ii == NULL) goto findDevicesCleanup;
    
    // perform bluetooth discovery, clear previously discovered devices from cache
    num_rsp = hci_inquiry(dev_id, DISC_UNIT, MAX_DEVS, NULL, &ii, IREQ_CACHE_FLUSH);
    if( num_rsp < 0 ) {
        printf("Discovery Notice: 0 Devices found.\n");
        goto findDevicesCleanup;
    }

    *devs = malloc(sizeof(devInf)* num_rsp);
    if (*devs == NULL) goto findDevicesCleanup;

    int i;
    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
            strcpy(name, "[unknown]");

        memcpy((*devs)[i].name, name, strlen(name));
        memcpy((*devs)[i].addr, addr, strlen(addr));
        (*devs)[i].name[strlen(name)] = '\0';
        (*devs)[i].addr[strlen(addr)] = '\0';
        printf("%s  %s\n", (*devs)[i].addr, (*devs)[i].name);
    }

    *numDevs = num_rsp;
    status = 0;

findDevicesCleanup:    
    if (ii) free( ii );
    if (sock >= 0) close( sock );

    return status;
}

int connect2Server(const int channel, const char* const dest, int * const err){
    struct sockaddr_rc addr = { 0 };
    int sock = -1, status;
    *err = -1;

    if (dest == NULL || strlen(dest) != 17 || err == NULL){
        printf("createClient Error: Invalid input.\n");
        goto connectCleanup;
    }

    // allocate a socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock == -1){
        printf("createClient Error: Cannot allocate socket. %d \n", errno);
        *err = errno;
        goto connectCleanup;
    }

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) channel;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    if (status == -1){
        printf("createClient Error: Cannot connect to socket. %d \n", errno);
        *err = errno;
        goto connectCleanup;
    }

    *err = 0;

connectCleanup:
    if (*err > 0 && sock >= 0) {
        close(sock);
        sock = -1;
    } // close socket if something went wrong

    return sock;
}

int sendReqWait4Resp(const int sock, const char * const reqData, const int size, char recData[CHUNK], int * const respSize){
    int status = -1, bytes_read = 0;
    *respSize = 0;

    if (sock < 0 || reqData == NULL || size <= 0){
        printf("sendRequest Error: Invalid Input.\n");
        goto sendRequestCleanup;
    }

    status = write(sock, reqData, size);

    if( status == -1 ){
        printf("sendRequest Error: Write error. %d \n", errno);
        status = errno;
        goto sendRequestCleanup;
    }

    bytes_read = read(sock, recData, CHUNK);
    if( bytes_read == -1 ) {
        printf("sendRequest Error: Failed to read message. %d \n", errno);
        status = errno;
        goto sendRequestCleanup;
    }

    *respSize = bytes_read;
    status = 0;

sendRequestCleanup:

    return status;
}

int client(const char* const dest, int channel, const char* const src, int sc, const char* const data, int size){
    int status = -1;
    struct sockaddr_rc dstAddr = { 0 };
    struct sockaddr_rc srcAddr = { 0 };
    int sock;
    fd_set fds;


    if (dest == NULL || strlen(dest) != 17){
        printf("Client Error: Invalid Dest Addr.\n");
        goto clientCleanup;
    }
    else if (data == NULL || size <= 0 || size > CHUNK){
        printf("Client Error: Invalid data.\n");
        goto clientCleanup;
    }


    // allocate a socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock == -1){
        printf("Client Error: Cannot allocate socket. %d \n", errno);
        status = errno;
        goto clientCleanup;
    }

    srcAddr.rc_family = AF_BLUETOOTH;
    srcAddr.rc_channel = (uint8_t) sc;
    str2ba( src, &srcAddr.rc_bdaddr );
    status = bind(sock, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
    if (status == -1){
        printf("Client Error: Cannot bind to local socket. %d \n", errno);
        status = errno;
        goto clientCleanup;
    }

    // set the connection parameters (who to connect to)
    dstAddr.rc_family = AF_BLUETOOTH;
    dstAddr.rc_channel = (uint8_t) channel;
    str2ba( dest, &dstAddr.rc_bdaddr );

    // connect to server
    status = connect(sock, (struct sockaddr *)&dstAddr, sizeof(dstAddr));

    if (status != 0 && errno == EHOSTDOWN){
        printf("Client Error: Can't connect to socket. Try again \n");
        if (sock >= 0) close(sock);
        else printf("sock %d\n", sock);
        sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        status = connect(sock, (struct sockaddr *)&dstAddr, sizeof(dstAddr));
    }
    else if (status != 0 && errno == ECONNREFUSED){
        channel++;
        dstAddr.rc_channel = (uint8_t) channel;
        if (sock >= 0) close(sock);
        else printf("sock %d\n", sock);
        sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        status = connect(sock, (struct sockaddr *)&dstAddr, sizeof(dstAddr));
    }

    if (status == 0){
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        status = select(sock + 1, NULL, &fds, NULL, NULL);

        if (status == -1){
            printf("Client Error: Write Select Failed. %d \n", errno);
            status = errno;
            goto clientCleanup;
        }

        status = write(sock, data, size);

        if( status == -1 ){
            printf("Client Error: Write error. %d \n", errno);
            status = errno;
            goto clientCleanup;
        }
    }
    else{
        printf("Client Error: Cannot connect to socket. %d \n", errno);
        status = errno;
        goto clientCleanup;
    }

    char buff[200];

    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    status = select(sock + 1, &fds, NULL, NULL, NULL);

    if (status == -1){
        printf("Client Error: Read Select Failed. %d \n", errno);
        status = errno;
        goto clientCleanup;
    }

    size = read(sock, buff, sizeof(buff));
    if( size == -1 ) {
        printf("Client Error: Failed to read message. %d \n", errno);
        status = errno;
        goto clientCleanup;
    }

    printf("Client Notice: Received: %d %s \n", size, buff);

    status = 0;

clientCleanup:
    if (sock >= 0) close(sock);

    return status;
}

int initServer(int * const err){
    int status = -1;
    struct sockaddr_rc loc_addr = { 0 };
    int sock = -1;
    *err = -1;

    if (err == NULL){
        printf("createServer Error: Data returned cannot be NULL \n");
        goto createServerCleanup;
    }

    // allocate socket
    sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (sock == -1){
        printf("createServer Error: Cannot allocate socket. %d \n", errno);
        *err = errno;
        goto createServerCleanup;
    }

    // bind socket to port 1 of the first available 
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
    loc_addr.rc_channel = (uint8_t) 1;
    status = bind(sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    if (status == -1){
        printf("createServer Error: Cannot bind name to socket. %d \n", errno);
        *err = errno;
        goto createServerCleanup;
    }

createServerCleanup:
    if (*err > 0 && sock >= 0) close (sock);

    return sock;
}

int listen4Req(int sock, char addr[ADDR_SIZE], int * const err){
    int status = -1, client = -1;
    struct sockaddr_rc clientAddr = { 0 };
    char cAddr[ADDR_SIZE] = { 0 };
    socklen_t opt = sizeof(clientAddr);
    *err = -1;

    if (sock < 0){
        printf("Listen Error: Invalid Input.\n");
        goto listen4ReqCleanup;
    }

    // put socket into listening modeq
    status = listen(sock, MAX_CON_DEVS);
    if (status == -1){
        printf("Listen Error: Cannot listen for connections on socket. %d \n", errno);
        *err = errno;
        goto listen4ReqCleanup;
    }

    // accept one connection
    client = accept(sock, (struct sockaddr *)&clientAddr, &opt);
    if (client == -1){
        printf("Listen Error: Failed to accept message. %d \n", errno);
        *err = errno;
        goto listen4ReqCleanup;
    }

    ba2str( &clientAddr.rc_bdaddr, cAddr );
    printf("Listen Notice: accepted connection from %s \n", cAddr);
    memcpy(addr, cAddr, ADDR_SIZE-1);
    addr[ADDR_SIZE-1] = '\0';

    status = 0;

listen4ReqCleanup:

    return client;
}

int fetchRequestData(int sock, char ** const data, int * const size){
    int status = -1;
    char buff[CHUNK] = { 0 };
    int bytesRead = 0;

    if (sock < 0 || data == NULL || size == NULL){
        printf("Receive Error: Invalid Input.\n");
        goto fetchRequestDataCleanup;
    }

    // read data from the client
    bytesRead = read(sock, buff, sizeof(buff));
    if( bytesRead == -1 ) {
        printf("Receive Error: Failed to read message. %d \n", errno);
        status = errno;
        goto fetchRequestDataCleanup;
    }

    printf("Receive Notice: Read message:\n%s\n", buff);
    *data = malloc(sizeof(char)*bytesRead);
    if (*data == NULL){
        printf("Receive Error: Failed to return data\n");
        status = -1;
        goto fetchRequestDataCleanup;
    }

    memcpy(*data, buff, bytesRead);
    status = 0;

fetchRequestDataCleanup:

    return status;
}

int sendResponse(int sock, const char * const data, const int size){
    int status = -1;

    if (sock < 0 || data == NULL){
        printf("Response Error: Invalid Input.\n");
        goto sendResponseCleanup;
    }

    status = write(sock, data, size);
    if( status == -1 ){
        printf("Response Error: Write error. %d \n", errno);
        status = errno;
        goto sendResponseCleanup;
    }

    printf("Response Notice: Wrote: %d %s \n", status, data);

    status = 0;

sendResponseCleanup:

    return status;
}

int closeSocket(int sock){
    int status = -1;

    if (sock < 0){
        printf("Socket Error: Invalid Input \n");
        return status;
    }

    return close(sock);
}

int server(char addr[ADDR_SIZE], const char* const myAddr, int channel, char ** const data, int* const size){
    int status = -1;
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    char buff[CHUNK] = { 0 };
    char cAddr[ADDR_SIZE] = { 0 };
    int s, client, bytes_read;
    fd_set fds;
    socklen_t opt = sizeof(rem_addr);

    if (data == NULL || size == NULL){
        printf("Server Error: Data returned cannot be NULL \n");
        goto serverCleanup;
    }

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (s == -1){
        printf("Server Error: Cannot allocate socket. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    // bind socket to port 1 of the first available 
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;
    //loc_addr.rc_bdaddr = *BDADDR_ANY;
    loc_addr.rc_channel = (uint8_t) channel;
    str2ba(myAddr, &loc_addr.rc_bdaddr);
    status = bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    if (status == -1){
        printf("Server Error: Cannot bind name to socket. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    // put socket into listening modeq
    status = listen(s, 1);
    if (status == -1){
        printf("Server Error: Cannot listen for connections on socket. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    // accept one connection
    client = accept(s, (struct sockaddr *)&rem_addr, &opt);
    if (client == -1){
        printf("Server Error: Failed to accept message. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    ba2str( &rem_addr.rc_bdaddr, cAddr );
    printf("Server Notice: accepted connection from %s \n", cAddr);
    memcpy(addr, cAddr, ADDR_SIZE-1);

    FD_ZERO(&fds);
    FD_SET(client, &fds);
    status = select(client + 1, &fds, NULL, NULL, NULL);

    if (status == -1){
        printf("Server Error: Read Select Failed. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    // read data from the client
    bytes_read = read(client, buff, sizeof(buff));
    if( bytes_read == -1 ) {
        printf("Server Error: Failed to read message. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    printf("Server Notice: Read message:\n%s %d\n", buff, bytes_read);
    *data = malloc(sizeof(char)*bytes_read);
    if (*data == NULL){
        printf("Server Error: Failed to return data\n");
        goto serverCleanup;
    }

    memcpy(*data, buff, bytes_read);
    status = 0;

    buff[0] = '!';
    buff[bytes_read-1] = '!';

    FD_ZERO(&fds);
    FD_SET(client, &fds);
    status = select(client + 1, NULL, &fds, NULL, NULL);

    if (status == -1){
        printf("Server Error: Write Select Failed. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    status = write(client, buff, bytes_read);

    if( status == -1 ){
        printf("Server Error: Write error. %d \n", errno);
        status = errno;
        goto serverCleanup;
    }

    printf("Server Notice: Wrote: %d %s \n", status, buff);

serverCleanup:
    close(client);
    close(s);

    return status;
}

#if DEBUG == 1
int main(int argc, char **argv)
{
    char addr[ADDR_SIZE] = {0};
    char src[ADDR_SIZE] = {0};
    int channel = 0;
    int sc = 0;
    char* data = NULL;
    devInf *devs = NULL;
    int size = 0;

    if (argc < 2){
        printf("Usage: ./a.out [-c [dstAddr] [dstCh] [srcAddr] [srcCh]]\n./a.out [-s [locAddr] [locCh]\n./a.out [-l | -f]\n");
        return 0;
    }
    else if (argc < 3){
        if (strcmp("-f", argv[1]) != 0 && strcmp("-l", argv[1]) != 0){
            printf("Usage: ./a.out [-c [dstAddr] [dstCh] [srcAddr] [srcCh]]\n./a.out [-s [locAddr] [locCh]\n./a.out [-l | -f]\n");
            return 0;
        }
    }
    else if (argc < 4){
        printf("Usage: ./a.out [-c [dstAddr] [dstCh] [srcAddr] [srcCh]]\n./a.out [-s [locAddr] [locCh]\n./a.out [-l | -f]\n");
        return 0;
    }
    else if (argc < 6){
        if (strcmp ("-c", argv[1]) == 0){
            printf("Usage: ./a.out [-c [dstAddr] [dstCh] [srcAddr] [srcCh]]\n./a.out [-s [locAddr] [locCh]\n./a.out [-l | -f]\n");
            return 0;
        }
        memcpy(&addr[0], argv[2], ADDR_SIZE);
        channel = atoi(argv[3]);
    }
    else{
        memcpy(&addr[0], argv[2], ADDR_SIZE);
        channel = atoi(argv[3]);
        memcpy(&src[0], argv[4], ADDR_SIZE);
        sc = atoi(argv[5]);
    }

    if (strcmp ("-c", argv[1]) == 0)
        client(&addr[0], channel, src, sc, "Send This Data", sizeof("Send This Data"));
    else if (strcmp ("-s", argv[1]) == 0)
        server(addr, &addr[0], channel, &data, &size);
    else if (strcmp ("-f", argv[1]) == 0)
        findDevices(&devs, &size);
    else if (strcmp ("-l", argv[1]) == 0)
        findLocalDevices(&devs, &size);

    int i;
    for (i = 0; i < size; i++){
        printf("%s  %s\n", devs[i].addr, devs[i].name);
    }
    
    if (data) free(data);
    if (devs) free(devs);

    return 0;
}
#endif