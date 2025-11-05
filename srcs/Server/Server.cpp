#include "../../include/Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iomanip>
#include <vector>


Server::Server(int port, const std::string &password) : _sock_fd(-1), _port(port), _password(password)
{
	createSocket();
	bindAndListen();
}

Server::~Server()
{
	if (_sock_fd != -1)
		close(_sock_fd);
	for (std::map<int, Client>::iterator iter = _clients.begin(); iter != _clients.end(); ++iter)
		close(iter->first);
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

	while (true) // Main server loop
	{
		if (poll(&fds[0], fds.size(), -1) < 0) // If poll fails
			throw std::runtime_error("Poll failed");
		
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
					std::cout << "Client connected, fd=" << client_fd << std::endl;
					fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set client socket to non-blocking
					struct pollfd pfd = {client_fd, POLLIN, 0}; // Prepare pollfd for new client
					fds.push_back(pfd); // Add new client to poll fds
					_clients[client_fd] = Client(); // Initialize new client data
				}
				/* Recv from client */
				else
				{
					char buf[1024];
					ssize_t bytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0); // Receive data
					if (bytes <= 0) // Close connection on error or disconnect
					{
						std::cout << "Client fd=" << fds[i].fd << " disconnected" << std::endl;
						close(fds[i].fd);
						_clients.erase(fds[i].fd);
						fds.erase(fds.begin() + i);
						--i;
						continue ;
					}
					buf[bytes] = '\0'; // Null-terminate the received data
					_clients[fds[i].fd].buffer += buf; // Append to buffer

					/* Loop to process complete commands */
					size_t pos;

					while ((pos = _clients[fds[i].fd].buffer.find('\n')) != std::string::npos)
					{
						std::string cmd = _clients[fds[i].fd].buffer.substr(0, pos);
						if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
							cmd.erase(cmd.size() - 1); // Remove trailing \r if present
						_clients[fds[i].fd].buffer.erase(0, pos + 1); // Remove processed command from buffer
						if (cmd.empty()) // Skip empty commands
							continue ;

						// PING cmd sends PONG back to client or echoes the command
						std::string resp;

						if (cmd.find("PASS ") == 0)
						{
							std::string pass = cmd.substr(5);
							if (pass == _password)
							{
								_clients[fds[i].fd].authenticated = true;
								resp = ":server 001 nick :Welcome to the IRC server\r\n";
							}
							else
								resp = ":server 464 :Password incorrect\r\n";
						}
						else if (_clients[fds[i].fd].authenticated)
						{
							if (cmd == "PING")
								resp = "PONG\r\n";
							else if (!cmd.empty())
								resp = cmd + "\r\n";
						}
						else
							resp = ":server 464 : Authenticate first\r\n";
						if (!resp.empty())
						{
							if (send(fds[i].fd, resp.c_str(), resp.size(), 0) < 0)
								std::cout << "Send failed: " << strerror(errno) << "\n" << std::endl;
							else
								std::cout << "Sent to FD " << fds[i].fd << " = " << resp;
						}
					}
				}
			}
		}
	}
}