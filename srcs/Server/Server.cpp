#include "../../include/Server.hpp"
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <iomanip>

Server::Server(int port, const std::string &password, bool debug_mode) : _sock_fd(-1), _port(port), _password(password), _debug_mode(debug_mode)
{
	createSocket();
	bindAndListen();
}

Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
}

void Server::createSocket()
{
	_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
	if (_sock_fd < 0)
		throw std::runtime_error("Socket creation failed");
	int opt = 1;
	setsockopt(_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Set socket options
	fcntl(_sock_fd, F_SETFL, O_NONBLOCK); // Set socket to non-blocking mode
}

void Server::bindAndListen()
{
	struct sockaddr_in addr; // Server address structure
	memset(&addr, 0, sizeof(addr)); // Zero out the structure
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(_port); // Convert port to network byte order
	addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address

	if (bind(_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) // Bind the socket
		throw std::runtime_error("Bind failed");;
	if (listen(_sock_fd, 5) < 0) // Listen for incoming connections
		throw std::runtime_error("Listen failed");
	
	std::cout << "Listening on port " << _port << std::endl;
}

void Server::run()
{
	std::vector<struct pollfd> fds(1);
	fds[0].fd = _sock_fd;
	fds[0].events = POLLIN;
	std::string buffer; // Data-Storage per client

	while (true) // Main server loop
	{
		if (poll(&fds[0], fds.size(), -1) < 0) // If poll fails
			throw std::runtime_error("Poll failed");
		std::cout << "Poll returned, nfds=" << fds.size() << std::endl; //DEBUG
		
		for (size_t i = 0; i < fds.size(); ++i) // While there are fds to process
		{
			if (fds[i].revents & POLLIN) // Check for incoming data
			{
				/* Accept new connection */
				if (fds[i].fd == _sock_fd)
				{
					struct sockaddr_in addr; // Client address structure
					socklen_t len = sizeof(addr);
					int client_fd = accept(_sock_fd, (struct sockaddr*)&addr, &len);
					std::cout << "New connection accepted: FD " << client_fd << std::endl; //DEBUG
					if (client_fd < 0)
					{
						std::cout << "Accept failed" << strerror(errno) << std::endl; //DEBUG
						continue ;
					}
					std::cout << "Client connected, fd=" << client_fd << std::endl; //DEBUG

					fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set client socket to non-blocking
					struct pollfd pfd = {client_fd, POLLIN, 0}; // Prepare pollfd for new client
					fds.push_back(pfd); // Add new client to poll fds
				}
				/* Recv from client */
				else
				{
					char buf[1024];
					ssize_t bytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0); // Receive data
					std::cout << "Received " << bytes << " bytes from fd " << fds[i].fd << std::endl; //DEBUG
					if (bytes <= 0) // Close connection on error or disconnect
					{
						std::cout << "Connection closed: FD " << fds[i].fd << std::endl; //DEBUG
						close(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i;
						continue ;
					}
					buf[bytes] = '\0'; // Null-terminate the received data
					std::cout << "Raw Data(hex) = "; // DEBUG
					for (ssize_t j = 0; j < bytes; ++j) // DEBUG
						std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)buf[j] << " "; // DEBUG
					std::cout << std::dec << std::endl; //DEBUG
					std::cout << "Raw Data(str) = " << buf << std::endl; //DEBUG
					buffer += buf; // Append to buffer
					std::cout << "Buffer state =" << buffer << std::endl; //DEBUG

					/* Loop to process complete commands */
					size_t pos;
					std::string delim = _debug_mode ? "\n" : "\r\n"; // DEBUG
					while ((pos = buffer.find('\n')) != std::string::npos)
					{
						std::string cmd = buffer.substr(0, pos);
						if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
							cmd.erase(cmd.size() - 1); // Remove trailing \r if present
						buffer.erase(0, pos + 1); // Remove processed command from buffer
						if (cmd.empty()) // Skip empty commands
							continue ;
						std::cout << "CMD = " << cmd << std::endl; //DEBUG

						// PING cmd sends PONG back to client or echoes the command
						std::string resp;
						if (cmd == "PING")
							resp = "PONG\r\n";
						else
							resp = cmd + "\r\n";
						if (send(fds[i].fd, resp.c_str(), resp.size(), 0) < 0)
							std::cout << "Send failed: " << strerror(errno) << "\n" << std::endl;
						else
							std::cout << "Sent = " << resp << "\n" << std::endl;
					}
				}
			}
		}
	}
}