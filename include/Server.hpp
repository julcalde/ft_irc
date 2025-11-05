#ifndef SERVER_HPP
# define SERVER_HPP

#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include "Client.hpp"
#include <stdexcept>

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