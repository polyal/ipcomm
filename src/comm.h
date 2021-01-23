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

using namespace std;

class Comm
{
public:
	Comm();
	~Comm();

	void createServerThread();

	int read();
	int write();

private:
	const int port = 9090;
	int serverSocket = -1;
	int commSocket = -1;

	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;

	queue<string> messages;
	atomic<bool> runServer = true;
	unique_ptr<thread> serverthread = nullptr;
	unique_ptr<thread> receivehread = nullptr;
	unique_ptr<thread> sendThread = nullptr;

	
	int err = 0;

	void server();
	bool createServer();
	void receive();
	void send();
	int createClient();

	int close();


};

#endif
