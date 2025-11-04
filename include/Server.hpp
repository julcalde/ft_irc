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
		int _sock_fd;
		int _port;
		std::string _password;
		bool _debug_mode; // To allow \n for testing

		void createSocket();
		void bindAndListen();

	public:
		Server(int port, const std::string &password, bool debug_mode = false);
		~Server();
		void run();
};

#endif