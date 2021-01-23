#include <iostream>
#include "comm.h"

using namespace std;


Comm::Comm()
{

}

Comm::~Comm()
{
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.s_addr = INADDR_ANY;
	this->serverAddr.sin_port = htons(port);
}

void Comm::createServerThread()
{
	this->serverthread = make_unique<thread>(&Comm::server, this);
}

void Comm::server()
{
	if (!createServer()){
		cout << "Error creating server" << endl;
		return;
	}

	this->receivehread = make_unique<thread>(&Comm::receive, this);
	this->sendThread = make_unique<thread>(&Comm::send, this);
	this->receivehread->join();
	this->sendThread->join();

}

bool Comm::createServer()
{
	this->serverSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (this->serverSocket == -1){
		cout << "ERR socket: " << errno << endl;
		this->err = errno;
		return false;
	}
	
	if(bind(this->serverSocket, reinterpret_cast<struct sockaddr*>(&this->serverAddr), sizeof(this->serverAddr)) < 0){
		cout << "ERR bind: " << errno << endl;
		this->err = errno;
		return false;
	}

	if (listen(this->serverSocket, 1) < 0){
		cout << "ERR bind: " << errno << endl;
		this->err = errno;
		return false;
	}
	cout << "Waiting for incoming connections..." << endl;
	
	int size = sizeof(struct sockaddr_in);
	this->commSocket = accept(this->serverSocket, reinterpret_cast<struct sockaddr*>(&this->clientAddr), (socklen_t*)&size);
	if (this->commSocket < 0){
		cout << "ERR bind: " << errno << endl;
		this->err = errno;
		return false;
	}
	cout << "Connection Accepted" << endl;
}

void Comm::receive()
{
	vector<char> buffer(this->buffSize);
	string message;
	while (this->runServer){
		while(::read(this->commSocket, &buffer[0], this->buffSize) > 0 ){
			string part(buffer.begin(), buffer.end());
			message.append(part);
			cout << "Reveived: " << part << endl;
		}
		this->messages.push(message);
		buffer.clear();
		message.clear();
	}
}

void Comm::send()
{
	while (this->runServer){
		while (!this->messages.empty()){
			string message = this->messages.front();
			::write(this->commSocket, message.data() , message.length());
			this->messages.pop();
		}
	}
}

int Comm::createClient()
{

}

int Comm::read()
{

}

int Comm::write()
{

}

int Comm::close()
{
	if (this->serverSocket >= 0)
		::close(this->serverSocket);
	if (this->commSocket >= 0)
		::close(this->commSocket);
}