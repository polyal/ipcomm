#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

using namespace std;

int client()
{
	int sock;
	struct sockaddr_in server;
	char server_reply[2000];
	string message = "Message from client";
	
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1){
		cout << "ERR socket: " << errno << endl;
		return errno;
	}
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8888 );

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		cout << "ERR connect: " << errno << endl;
		return errno;
	}

	//Send some data
	if (send(sock, message.data() , message.length(), 0) < 0){
		cout << "ERR send: " << errno << endl;
		return errno;
	}
	
	//Receive a reply from the server
	if(read(sock, server_reply, 2000) < 0){
		cout << "ERR read: " << errno << endl;
		return errno;
	}
	
	cout << "Reply: " << server_reply << endl;
	
	close(sock);
	return 0;
}


int server()
{ 
    int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		cout << "ERR socket: " << errno << endl;
		return errno;
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );
	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
		cout << "ERR bind: " << errno << endl;
		return errno;
	}

	//Listen
	listen(socket_desc, 3);
	
	//Accept and incoming connection
	cout << "Waiting for incoming connections..." << endl;
	c = sizeof(struct sockaddr_in);
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0){
		cout << "ERR bind: " << errno << endl;
		return errno;
	}
	cout << "Connection Accepted" << endl;
	
	//Receive a message from client
	while((read_size = read(client_sock, client_message, 2000)) > 0 ){
		//Send the message back to client
		write(client_sock, client_message , strlen(client_message));
	}
	
	if(read_size == 0){
		cout << "Client disconnected" << errno << endl;
		fflush(stdout);
	}
	else if(read_size == -1){
		cout << "recv failed" << errno << endl;
	}
	
	return 0;
}


int main()
{
	cout << "works" << endl;
	return 0;
}