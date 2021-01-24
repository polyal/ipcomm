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
	Comm(const string& ip);
	~Comm();

	void createServer();
	void createClient();
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
	atomic<bool> run = false;
	bool printMessages = false;
	bool sendMessages = false;
	recursive_mutex outputm;
	recursive_mutex inputm;
	condition_variable_any outputcv;
	condition_variable_any inputcv;
	unique_ptr<thread> serverThread = nullptr;
	unique_ptr<thread> clientThread = nullptr;
	unique_ptr<thread> receiveThread = nullptr;
	unique_ptr<thread> sendThread = nullptr;
	unique_ptr<thread> printThread = nullptr;
	
	int err = 0;

	void createServerThread();
	void server();
	bool initServer();
	void createClientThread();
	void client();
	bool initClient();
	void createWorkerThreads();
	void joinWorkerThreads();
	void receive();
	void send();
	void print();
	
	int close();
};

#endif
