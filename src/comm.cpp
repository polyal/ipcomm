#include <iostream>
#include "channel.h"
#include "comm.h"

using namespace std;


Comm::Comm()
{
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.s_addr = INADDR_ANY;
	this->serverAddr.sin_port = htons(this->port);
}

Comm::Comm(const string& ip)
{
	this->serverAddr.sin_family = AF_INET;
	this->serverAddr.sin_addr.s_addr = inet_addr(ip.data());
	this->serverAddr.sin_port = htons(this->port);
}

Comm::~Comm()
{
	close();
	kill();
}

void Comm::sendMessage(const string& message)
{
	unique_lock<recursive_mutex> lock(this->inputm);
	this->messages.push(message);
	this->sendMessages = true;
	lock.unlock();
	this->inputcv.notify_one();
}

void Comm::createServer()
{
	createServerThread();
}

void Comm::createServerThread()
{
	this->serverThread = make_unique<thread>(&Comm::server, this);
}

void Comm::server()
{
	if (!initServer()){
		close();
		cout << "Error creating server: " << this->err << endl;
		return;
	}
	createWorkerThreads();
}

bool Comm::initServer()
{
	this->serverSocket = socket(AF_INET , SOCK_STREAM , 0);
	if (this->serverSocket == -1){
		cout << "ERR socket: " << errno << endl;
		this->err = errno;
		return false;
	}
	
	if (bind(this->serverSocket, reinterpret_cast<struct sockaddr*>(&this->serverAddr), sizeof(this->serverAddr)) < 0){
		cout << "ERR bind: " << errno << endl;
		this->err = errno;
		return false;
	}

	if (listen(this->serverSocket, 1) < 0){
		cout << "ERR listen: " << errno << endl;
		this->err = errno;
		return false;
	}

	cout << "Waiting for incoming connections..." << endl;
    while (!waitReadyOrTimeout(this->serverSocket)){
	    if (!this->run || this->err > 0)
	    	return false;
	}

    int size = sizeof(struct sockaddr_in);
	this->commSocket = accept(this->serverSocket, reinterpret_cast<struct sockaddr*>(&this->clientAddr), (socklen_t*)&size);
	if (this->commSocket < 0){
		cout << "ERR accept: " << errno << endl;
		this->err = errno;
		return false;
	}
	cout << "Connection Accepted" << endl;
    cout << "New connection : "
    << "[SOCKET_FD : " << this->commSocket
    << " , IP : " << inet_ntoa(this->clientAddr.sin_addr)
    << " , PORT : " << ntohs(this->clientAddr.sin_port)
    << "]" << endl;
	return true;
}

void Comm::createClient()
{
	createClientThread();
}

void Comm::createClientThread()
{
	this->clientThread = make_unique<thread>(&Comm::client, this);
	if (this->clientThread)
		this->clientThread->join();
}

void Comm::client()
{
	if (!initClient()){
		cout << "Error creating client: " << this->err << endl;
		close();
		return;
	}
	createWorkerThreads();
}

bool Comm::initClient()
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

void Comm::receive()
{
	while (this->run){
		while (!waitReadyOrTimeout(this->commSocket)){
	    	if (!this->run || this->err > 0)
	    		return;
		}
		cout << "READ ready" << endl; 
		vector<char> buffer(this->buffSize);
		string reply;
		int bytesRead;
		while ((bytesRead = ::read(this->commSocket, &buffer[0], this->buffSize)) > 0){
	        string part(buffer.begin(), buffer.end());
			reply.append(part);
			buffer.clear();
	        if (bytesRead < (int)this->buffSize || (errno != 0 && errno != ENOENT))
	            break;
	    }
	    if(bytesRead == -1){
	        cout << "Err read: " << errno << endl;
	        this->err = errno;
	        return;
	    }
	    else if (bytesRead == 0){
	    	cout << "READ closed connection from server" << endl;
	    	::close(this->commSocket);
	    }
	    this->replies.push(reply);

		unique_lock<recursive_mutex> lock(this->outputm);
		int locked = true;
		if (!this->printMessages){
			while (!this->replies.empty()){
				string message = this->replies.front();
				this->printingQueue.push(message);
				this->replies.pop();
			}
			this->printMessages = true;
			lock.unlock();
			locked = false;
			this->outputcv.notify_one();
		}
		if (locked)
			lock.unlock();
	}
}

void Comm::send()
{
	while (this->run){
		unique_lock<recursive_mutex> lock(this->inputm);
		inputcv.wait(lock, [this]{ return this->sendMessages;});
		cout << "SEND ready" << endl;
		while (!this->messages.empty()){
			string message = this->messages.front();
			if (::write(this->commSocket, message.data() , message.length()) == -1){
				cout << "Err write: " << errno << endl;
				::close(this->commSocket);
			}
			cout << "Sending: " << message << endl;
			this->messages.pop();
		}
		this->sendMessages = false;
		lock.unlock();
	}
}

void Comm::print()
{
	while (this->run){
		unique_lock<recursive_mutex> lock(this->outputm);
		cout << "PRINT " << endl;
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

bool Comm::waitReadyOrTimeout(const int fd)
{
	fd_set readfds;
	FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int ret;
    if ((ret = select(fd + 1, &readfds, NULL, NULL, &timeout)) < 0){
        cout << "ERR select: " << errno << endl;
        if (errno == EINTR){
        	cout << "EINTR" << endl;
        }
        this->err = errno;
        return false;
    }
    cout << "TIME " << ret << " " << timeout.tv_sec << " " << timeout.tv_usec << endl;
    if (!FD_ISSET(fd, &readfds)) {
    	cout << "ERR FD_ISSET: " << errno << endl;
		this->err = errno;
		return false;
    }
    cout << "WAIT " << this->err << endl;
    return true;
}

void Comm::createWorkerThreads()
{
	this->receiveThread = make_unique<thread>(&Comm::receive, this);
	this->sendThread = make_unique<thread>(&Comm::send, this);
	this->printThread = make_unique<thread>(&Comm::print, this);
}

void Comm::kill()
{
	killThreadLoops();
	signalWaitingThreads();
	joinWorkerThreads();
}

void Comm::joinWorkerThreads()
{
	if (this->serverThread)
		this->serverThread->join();
	if (this->receiveThread)
		this->receiveThread->join();
	if (this->sendThread)
		this->sendThread->join();
	if (this->printThread)
		this->printThread->join();
}

void Comm::signalWaitingThreads()
{
	unique_lock<recursive_mutex> sendLock(this->inputm);
	this->sendMessages = true;
	sendLock.unlock();
	this->inputcv.notify_one();
	unique_lock<recursive_mutex> printLock(this->outputm);
	this->printMessages = true;
	printLock.unlock();
	this->outputcv.notify_one();
}

void Comm::killThreadLoops()
{
	this->run = false;
}

bool Comm::isRunning() const
{
	return this->run;
}

int Comm::close()
{
	int res = 0;
	if (this->serverSocket >= 0){
		res = ::close(this->serverSocket);
		this->serverSocket = -1;
	}
	if (this->commSocket >= 0){
		res = res || ::close(this->commSocket);
		this->commSocket = -1;
	}
	return res;
}
