#ifndef SERVER_HPP
# define SERVER_HPP

#include "Client.hpp"
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>
#include <stdexcept>
#include <signal.h>
#include <poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <map>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>

class Server
{
	private:
		struct Client
		{
			std::string buffer;
			bool authenticated;
			Client() : authenticated(false) {} // Sets authenticated to false by default
		};
		
		int _sock_fd;
		int _port;
		std::string _password;
		std::map<int, Client> _clients; // Map of client FDs to Client data


		void createSocket();
		void bindAndListen();

	public:
		Server(int port, const std::string &password);
		~Server();
		void run();
};

#endif