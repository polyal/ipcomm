#include <iostream>
#include "comm.h"

using namespace std;


Comm::Comm()
{
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.s_addr = INADDR_ANY;
	this->serverAddr.sin_port = htons(port);
}

Comm::Comm(const string& ip, const int port)
{
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.s_addr = inet_addr(ip.data());
	this->serverAddr.sin_port = htons(port);
}

Comm::~Comm()
{
	close();
}

void Comm::sendMessage(const string& message)
{
	unique_lock<recursive_mutex> lock(this->inputm);
	this->messages.push(message);
	this->sendMessages = true;
	lock.unlock();
	this->inputcv.notify_one();
}

void Comm::createServerThread()
{
	this->serverthread = make_unique<thread>(&Comm::server, this);
}

void Comm::server()
{
	if (!createServer()){
		close();
		cout << "Error creating server: " << this->err << endl;
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
	return true;
}

void Comm::receive()
{
	vector<char> buffer(this->buffSize);
	string reply;
	while (this->runServer){
		while(::read(this->commSocket, &buffer[0], this->buffSize) > 0 ){
			string part(buffer.begin(), buffer.end());
			reply.append(part);
			cout << "Reveived: " << part << endl;
		}
		this->replies.push(reply);
		buffer.clear();
		reply.clear();

		unique_lock<recursive_mutex> lock(this->outputm);
		int locked  = true;
		if (!this->printMessages){
			while (!this->replies.empty()){
				string message = this->replies.front();
				::write(this->commSocket, message.data(), message.length());
				this->printingQueue.push(message);
				this->replies.pop();
			}
			this->printMessages = true;
			lock.unlock();
			locked = false;
			outputcv.notify_one();
		}
		if (locked)
			lock.unlock();
	}
}

void Comm::send()
{
	while (this->runServer){
		unique_lock<recursive_mutex> lockin(this->inputm);
		inputcv.wait(lockin, [this]{ return this->sendMessages;});
		unique_lock<recursive_mutex> lockout(this->outputm);
		int locked = true;

		if (!this->printMessages){
			while (!this->messages.empty()){
				string message = this->messages.front();
				::write(this->commSocket, message.data() , message.length());
				this->printingQueue.push(message);
				this->messages.pop();
			}
			this->printMessages = true;
			this->sendMessages = false;
			lockout.unlock();
			locked = false;
			outputcv.notify_one();
		}
		if (locked)
			lockout.unlock();
		lockin.unlock();
	}
}

void Comm::print()
{
	while (this->runServer){
		unique_lock<recursive_mutex> lock(this->outputm);
		this->outputcv.wait(lock, [this]{ return this->printMessages;});

		while (!printingQueue.empty()){
			string message = printingQueue.front();
			cout << message << endl;
			printingQueue.pop();
		}

		this->printMessages = false;
		lock.unlock();
	}
}

void Comm::createClientThread()
{
	this->clientthread = make_unique<thread>(&Comm::client, this);
}

bool Comm::createClient()
{	
	this->commSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->commSocket == -1){
		cout << "ERR socket: " << errno << endl;
		this->err = errno;
		return false;
	}

	if (connect(this->commSocket, reinterpret_cast<struct sockaddr*>(&this->serverAddr), sizeof(this->serverAddr)) < 0){
		cout << "ERR connect: " << errno << endl;
		this->err = errno;
		return false;
	}
	return true;
}

void Comm::client()
{
	if (!createClient()){
		cout << "Error creating client: " << this->err << endl;
		close();
		return;
	}

	this->receivehread = make_unique<thread>(&Comm::receive, this);
	this->sendThread = make_unique<thread>(&Comm::send, this);
	this->receivehread->join();
	this->sendThread->join();
}

int Comm::close()
{
	int res = 0;
	if (this->serverSocket >= 0)
		res = ::close(this->serverSocket);
	if (this->commSocket >= 0)
		res = res || ::close(this->commSocket);
	return res;
}
