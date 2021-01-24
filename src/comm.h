#ifndef COMM_H
#define COMM_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std;


class Comm
{
public:
	Comm();
	Comm(const string& ip, const int port);
	~Comm();

	void createServerThread();
	void createClientThread();
	void sendMessage(const string& message);

private:
	const int port = 9090;
	const int buffSize = 1024;
	int serverSocket = -1;
	int commSocket = -1;

	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;

	queue<string> messages;
	queue<string> replies;
	queue<string> printingQueue;
	atomic<bool> runServer = true;
	bool printMessages = false;
	bool sendMessages = false;
	recursive_mutex outputm;
	recursive_mutex inputm;
	condition_variable_any outputcv;
	condition_variable_any inputcv;
	unique_ptr<thread> serverthread = nullptr;
	unique_ptr<thread> clientthread = nullptr;
	unique_ptr<thread> receivehread = nullptr;
	unique_ptr<thread> sendThread = nullptr;
	
	int err = 0;

	void server();
	bool createServer();
	void client();
	bool createClient();
	void receive();
	void send();
	void print();
	
	int close();
};

#endif
