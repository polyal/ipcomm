#include <iostream>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <string>

using namespace std;


const unsigned short port = 8080;
const string hello = "Hello from server";


int server()
{ 
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
        cout << "ERR socket: " << errno << endl; 
        return errno;
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){ 
        cout << "ERR setsockopt: " << errno << endl; 
        return errno; 
    } 

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port );    
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){ 
        cout << "ERR bind: " << errno << endl;
        return errno;
    } 

    if (listen(server_fd, 3) < 0){ 
        cout << "ERR listen: " << errno << endl; 
        return errno;
    } 

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0){ 
        cout << "ERR accept: " << errno << endl; 
        return errno;
    }

    valread = read( new_socket , buffer, 1024); 
    cout << "Message: " << buffer << endl;
    send(new_socket , hello.data() , hello.length(), 0 ); 
    cout << "Hello message sent" << endl;
    return 0;
}


int main()
{
	cout << "works" << endl;
	return 0;
}