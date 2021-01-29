#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

using namespace std;

class Channel
{
public:
	Channel(const string& ip, const int port);

protected:
	string ip = "127.0.0.1";
	int port = 80;
	int backlog = 10;
	size_t buffSize = 1024;

	struct sockaddr_in s_Addr;
	struct sockaddr_in c_Addr;
	int s_fd = -1;
	int c_fd = -1;


	int salloc();
	int bind();
	int listen();
	int accept();
	int connect();
	int close();

	int read();
	int write();
};
