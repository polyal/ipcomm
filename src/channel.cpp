#include <iostream>
#include <vector>
#include "channel.h"

using namespace std;


Channel::Channel(const string& ip, const int port) : ip{ip}, port{port}
{
	this->s_Addr.sin_family = AF_INET;
	this->s_Addr.sin_addr.s_addr = INADDR_ANY;
	this->s_Addr.sin_port = htons(port);
}

int Channel::salloc()
{
	int res = 0;
	this->s_fd = socket(AF_INET , SOCK_STREAM , 0);
	if (this->s_fd == -1){
		cout << "ERR socket: " << errno << endl;
		res = errno;
	}
	return res;
}

int Channel::bind()
{
	int res = 0;
	if (::bind(this->s_fd, reinterpret_cast<struct sockaddr*>(&this->s_Addr), sizeof(this->s_Addr)) < 0){
		cout << "ERR bind: " << errno << endl;
		res = errno;
	}
	return res;
}

int Channel::listen()
{
	int res = 0;
	if (::listen(this->s_fd, this->backlog) < 0){
		cout << "ERR listen: " << errno << endl;
		res = errno;
	}
	return res;
}

int Channel::accept()
{
	int res = 0, size = sizeof(struct sockaddr_in);
	this->c_fd = ::accept(this->s_fd, reinterpret_cast<struct sockaddr*>(&this->c_Addr), (socklen_t*)&size);
	if (this->c_fd < 0){
		cout << "ERR accept: " << errno << endl;
		res = errno;
	}
	return res;
}

int Channel::connect()
{
	int res = 0;
	if (::connect(this->c_fd, reinterpret_cast<struct sockaddr*>(&this->s_Addr), sizeof(this->s_Addr)) < 0){
		cout << "ERR connect: " << errno << endl;
		res = errno;
	}
	return res;
}

int Channel::read()
{
	vector<char> buffer(this->buffSize);
	string reply;
	int res = 0, bytesRead = 0;
	while ((bytesRead = ::read(this->c_fd, &buffer[0], this->buffSize)) > 0){
        string part(buffer.begin(), buffer.end());
		reply.append(part);
		buffer.clear();
        if (bytesRead < (int)this->buffSize || (errno != 0 && errno != ENOENT))
            break;
    }
    if(bytesRead == -1){
        cout << "Err read: " << errno << endl;
        res = errno;
    }
    else if (bytesRead == 0){
    	cout << "READ closed connection from server" << endl;
    }
    return res;
}

int Channel::write()
{
	vector<char> buffer(this->buffSize);
	int res = 0;
	if (::write(this->c_fd, &buffer[0], this->buffSize) == -1){
		cout << "Err write: " << errno << endl;
		res = errno;
	}
	return res;
}
