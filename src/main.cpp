#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include "comm.h"

using namespace std;

int client(const string& ip)
{
	int sock, err = 0;
	struct sockaddr_in server;
	char server_reply[2000];
	string message = "Message from client";
	
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1){
		cout << "ERR socket: " << errno << endl;
		err = errno;
		goto cleanup;
	}
	
	server.sin_addr.s_addr = inet_addr(ip.data());
	server.sin_family = AF_INET;
	server.sin_port = htons( 9090 );

	//Connect to remote server
	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		cout << "ERR connect: " << errno << endl;
		err = errno;
		goto cleanup;
	}

	//Send some data
	if (send(sock, message.data() , message.length(), 0) < 0){
		cout << "ERR send: " << errno << endl;
		err = errno;
		goto cleanup;
	}
	
	//Receive a reply from the server
	if(read(sock, server_reply, 2000) < 0){
		cout << "ERR read: " << errno << endl;
		err = errno;
		goto cleanup;
	}
	
	cout << "Reply: " << server_reply << endl;
	
cleanup:
	close(sock);
	return err;
}


int server()
{ 
    int socket_desc, client_sock, c, read_size, err = 0;
	struct sockaddr_in server, client;
	char client_message[2000];
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		cout << "ERR socket: " << errno << endl;
		err = errno;
		goto cleanup;
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 9090 );
	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
		cout << "ERR bind: " << errno << endl;
		err = errno;
		goto cleanup;
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
		err = errno;
		goto cleanup;
	}
	cout << "Connection Accepted" << endl;
	
	//Receive a message from client
	while((read_size = read(client_sock, client_message, 2000)) > 0 ){
		//Send the message back to client
		cout << "Reveived: " << client_message << endl;
		write(client_sock, client_message , strlen(client_message));
	}
	
	if(read_size == 0){
		cout << "Client disconnected" << errno << endl;
		fflush(stdout);
	}
	else if(read_size == -1){
		cout << "recv failed" << errno << endl;
	}

cleanup:
	close(socket_desc);
	close(client_sock);

	return err;
}


int main(int argc,char* argv[])
{
	if (argc < 2){
		cout << "Usage: a.out [c[ip]|s]" << endl;
		return 0;
	}

	string arg1{argv[1]};
	if (arg1 == "s"){
		//server();
		Comm comm;
		comm.createServer();
		while (comm.isRunning()){
			string in;
			getline(cin, in);
			if (in == ":q")
				break;
			comm.sendMessage(in);
		}
	}
	else if (arg1 == "c"){
		if (argc < 3){
			cout << "Usage: a.out [c[ip]|s]" << endl;
			return 0;
		}
		string arg2{argv[2]};
		client(arg2);

		Comm comm{arg2};
		comm.createClient();
		while (comm.isRunning()){
			string in;
			getline(cin, in);
			if (in == ":q")
				break;
			comm.sendMessage(in);
		}
	}
	else{
		cout << arg1 << " is an invalid argument" << endl;
		cout << "Usage: a.out [c|s]" << endl;
	}


	return 0;
}
