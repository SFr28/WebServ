#include "serverSocket.hpp"

extern int	g_signal;

///////////////////////// Constructors/Destructor ///////////////////////

serverSocket::serverSocket(Config& config) : _serverConfig(config.getServer())
{
	this->_epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (this->_epollfd == -1)
		throw serverSocket::SocketException("Error: cannot create epoll\n");

	int opt = 1;
	int i = 0;
	
	std::vector<ServerParse>	servers = config.getServer();

	for (std::vector<ServerParse>::iterator it = servers.begin(); it < servers.end(); it++)
	{
		struct sockaddr_in add_tmp;
		std::string ad = it->getHost();
		unsigned long add = inet_addr(ad.c_str());
		this->_serverfd.push_back(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
		if (this->_serverfd.back() < 0)
			throw serverSocket::SocketException("Error: cannot create socket\n");
		
		add_tmp.sin_family = AF_INET;
		add_tmp.sin_port = htons(std::atoi(it->getListen().c_str()));
		add_tmp.sin_addr.s_addr = add;
		bzero(add_tmp.sin_zero, sizeof(add_tmp.sin_zero));
		this->_serverAddress.push_back(add_tmp);
	
		setsockopt(this->_serverfd.back(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		i++;
	}
	this->_nbServer = i;
}

serverSocket::~serverSocket()
{
	std::vector<int>::iterator	it;

	for (int j = 0; j < this->_nbServer; j++)
	{
		if (this->_serverfd[j] > 0)
			close (this->_serverfd[j]);
	}
	if (this->_epollfd > 0)
		close (this->_epollfd);
	
	for (std::map<int, HandleCGI*>::iterator it = _cgi.begin(); it != _cgi.end(); ++it)
	{
		delete it->second->getResponse();
		delete it->second;
	}
	_cgi.clear();
}

///////////////////////// Getters and setters ///////////////////////

int serverSocket::getServerfd(int index) const
{
	return (this->_serverfd[index]);
}
int serverSocket::get_epollfd() const
{
	return (this->_epollfd);
}

struct sockaddr_in serverSocket::getServerAddress(int index) const
{
	return (this->_serverAddress[index]);
}

std::map<std::string, std::string> &	serverSocket::getCookies(int index)
{
	return (this->_cookies[index]);
}

const std::vector<ServerParse> &	serverSocket::getServerConfig() const
{
	return (this->_serverConfig);
}

void serverSocket::setReadingStatus(bool status, int index, int fd)
{
	this->_readingStatus[index][fd] = status;
}

void serverSocket::setCookies(std::map<std::string, std::string> & cookies, int index)
{
	this->_cookies[index] = cookies;
}

///////////////////////// Initializing server ///////////////////////

//binding & listening with the server socket
//creating the epoll instance to stock all the clients' fd
//initializing all serverSocket's variables
void serverSocket::initialize()
{

	std::cout << B_CYAN << "Welcome to a Webserv by Lilou, Saina and Solenne !" << RESET << std::endl;

	for (int j = 0; j < this->_nbServer; j++)
	{
		struct sockaddr_in add_tmp = _serverAddress[j];
		socklen_t length = sizeof(add_tmp);
	
		if (bind(_serverfd[j], (struct sockaddr*)&add_tmp, length) < 0)
			throw serverSocket::SocketException("Error: cannot connect socket\n");

		if (listen(_serverfd[j], SOMAXCONN) < 0)
			std::cerr << RED << "Error: socket listen failed" << RESET << std::endl;
		
		epoll_event event;
		memset(&event, 0, sizeof(event));
		event.data.fd = _serverfd[j];
		event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
		if(epoll_ctl(this->_epollfd, EPOLL_CTL_ADD, _serverfd[j], &event) < 0)
			throw serverSocket::SocketException("Error: epoll_ctl\n");
		this->_readingStatus.push_back(std::map<int, bool>());
		this->_cookies.push_back(std::map<std::string, std::string>());
		this->_request.push_back(std::map<int, std::vector<char> >());
		this->_contentLength.push_back(std::map<int, size_t>());
		this->_clientsfd.push_back(std::vector<int>());
		this->_lastActivity.push_back(std::map<int, time_t>());
	}

	execution_loop();
}

///////////////////////// Server execution ///////////////////////

//////////////////// Sending requests //////////////////

//in case of timeout
static void send_timeoutRequest(int client_fd, serverSocket &server, int index)
{	
	httpRequest		httpRequest(GATEWAY_TIMEOUT, server.getServerConfig());
	
	httpResponse	httpResponse(httpRequest, server.getCookies(index));
	httpResponse.generateResponse();
	
	std::vector<char>	rawResponse = httpResponse.getResponse();

	if (write(client_fd, &rawResponse[0], httpResponse.getLenResponse()) < 0)
	{
		std::cerr << "Failed to send timeout response to client " << client_fd << "\n";
	}
	server.close_fd(client_fd, index);
	server.setReadingStatus(true, index, client_fd);
	server.setCookies(httpResponse.getCookies(), index);
}

//send normal request
//check if the request is complete (if not, don't handle it and wait for the next part)
//generate a response according to the request
//write the response, close the client and set the cookie
void serverSocket::send_request(std::vector<char> &request, int client_fd, bool connection_status, int index)
{	
	httpRequest		httpRequest(request, this->getServerConfig());
	this->_contentLength[index][client_fd] = httpRequest.getContentLength();
	
	if (connection_status == true && httpRequest.getIsParsed() == false)
	{
		this->setReadingStatus(false, index, client_fd);
		return ;
	}

	std::cout << GREEN << "Client number " << client_fd << " from server " << this->getServerfd(index) << " request is:\n" << RESET << std::endl;
	std::cout << WHITE << httpRequest.getprintRequest() << RESET << std::endl;

	this->_lastActivity[index][client_fd] = time(NULL);
	httpResponse* 	response = new httpResponse(httpRequest, this->getCookies(index));

	if (response->getIsCGI() == true)
	{
		HandleCGI*	cgi = new HandleCGI(httpRequest, response->getServer(), response , client_fd);
		
		cgi->startCGI();
		std::map<int, std::vector<char> >::iterator it = this->_request[index].find(client_fd);
		if (it ==  this->_request[index].end())
			std::cerr << RED << "Client not found" << RESET << std::endl;
		else
			it->second.clear();
		
		if (response->getStatusCode() == OK)
		{
			this->setReadingStatus(true, index, client_fd);
			this->_cgi[cgi->getFdOut()] = cgi;
			epoll_event ev;
			memset(&ev, 0, sizeof(ev));
			ev.data.fd = cgi->getFdOut();
			ev.events = EPOLLIN;
			if (epoll_ctl(this->_epollfd, EPOLL_CTL_ADD, cgi->getFdOut(), &ev) == -1)
				throw serverSocket::SocketException("Error: epoll_ctl\n");
			return;
		}
		
		delete cgi;
		response->handleError();
	}

	response->generateResponse();
	
	std::vector<char>	rawResponse = response->getResponse();
	if (!rawResponse.empty()) 
	{
		if (write(client_fd, &rawResponse[0], response->getLenResponse()) < 0) 
		{
			std::cerr << "Failed to send response to client " << client_fd << "\n";
		}
	}

	this->close_fd(client_fd, index);
	this->setReadingStatus(true, index, client_fd);
	this->setCookies(response->getCookies(), index);
	delete response;
}

//the CGI is finished, get the result and generate the response
void serverSocket::responding_to_cgi(int fd)
{
	int status = this->_cgi[fd]->parent_process();
	httpResponse*	response = this->_cgi[fd]->getResponse();
	
	if (status != OK)
		response->handleCGIError();
	else
		response->finalizeCGIResponse(this->_cgi[fd]->getBodyResponse());
	
	response->generateResponse();
	std::vector<char>	rawResponse = response->getResponse();
	if (write(this->_cgi[fd]->getClientFd(), &rawResponse[0], response->getLenResponse()) < 0)
		std::cerr << "Failed to send response to client " << fd << "\n";

	int index = get_server_index_from_fd(this->_cgi[fd]->getClientFd());
	if (index == -1)
	{
		std::cerr << "Unknown fd in epoll: " << fd << "\n";
		return ;
	}

	close_fd(fd, index);
	setReadingStatus(true, index, fd);
	setCookies(response->getCookies(), index);
	delete response;
	delete this->_cgi[fd];
	this->_cgi.erase(fd);
}

//////////////////// Execution //////////////////

//waiting for I/O events from clients and stocking new clients in our epoll
//if : it's a CGI => handle accordingly
//if : there is an event, but an error condition happened on the associated fd
//   : there is an event, but a hang up happened on the associated fd (the client closed its end)
//   : there isn't an event and epoll_ctl is in reading mode
//else if: the current fd is the server's => accept clients
//else if: it's a client's fd and it's ready to be read => read request
//handle timeout
void serverSocket::execution_loop()
{
	epoll_event events[SOMAXCONN];
	memset(&events, 0, sizeof(events));
	int status = 0;

	while (g_signal == 0)
	{
		int nevents = epoll_wait(this->_epollfd, events, SOMAXCONN, 1000);
		if (nevents < 0 && g_signal == 0)
			throw serverSocket::SocketException("Error: epoll_wait\n");

		for (int j = 0; j < nevents; j++)
		{
			int fd = events[j].data.fd;
			if (this->_cgi.find(fd) != this->_cgi.end())
			{
				pid_t w = waitpid(this->_cgi[fd]->getPID(), &status, WNOHANG);
				if (w != 0)
				{
					this->_cgi[fd]->setw(w);
					this->_cgi[fd]->setExecStatus(status);
					responding_to_cgi(fd);
				}
				continue;
			}
			
			int index = get_server_index_from_fd(fd);
			if (index == -1)
			{
				std::cerr << "Unknown fd in epoll: " << fd << "\n";
				continue;
			}

			if ((events[j].events & EPOLLERR) || (events[j].events & EPOLLRDHUP) || (events[j].events & EPOLLHUP))
				close_fd(fd, index);
			else if (check_server(fd))
				server_loop(index);
			else if (events[j].events & EPOLLIN && events[j].events & EPOLLOUT)
			{
				read_client(events, j, index);
				if (this->_lastActivity[index].find(fd) != this->_lastActivity[index].end())
					this->_lastActivity[index][fd] = time(NULL);
			}
		}
		checkTimeout();
	}
	std::cout << RED << "\nDisconnecting\n" << RESET << std::endl;
}

//Check the last activity of all client on the server
//If no new activity has been registered since more than 10sec and the reading 
//of the request is not complete, the request is closed and a timeout response is send
void	serverSocket::checkTimeout()
{
	const time_t timeout = 10;

	for (int i = 0; i < this->_nbServer; i++)
	{
		std::vector<int> to_close;
		time_t now = time(NULL);

		for (std::map<int, time_t>::iterator it = this->_lastActivity[i].begin(); it != this->_lastActivity[i].end(); )
		{
			int fd = it->first;
		
			if (now - it->second > timeout && this->_readingStatus[i][fd] == false)
			{
				std::cout << B_RED << "Timeout..." << RESET << std::endl << std::endl;
				to_close.push_back(fd);
				it++;
				continue; 
			}
			else
				it++;
		}

		for (size_t j = 0; j < to_close.size(); j++)
			send_timeoutRequest(to_close[j], *this, i);
	}
}

//in the server socket, accept incoming client
//set it in reading and writing
//add it to the interest list (EPOLL_CTL_ADD)
void serverSocket::server_loop(int index)
{
	sockaddr_in in_addr;
	socklen_t len = sizeof(in_addr);

	int client = accept(_serverfd[index], (struct sockaddr*)&in_addr, &len);
	
	if (client < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		throw serverSocket::SocketException("Error: cannot accept client\n");
	else if (client < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		return ;
		
	else
	{
		epoll_event event;
		memset(&event, 0, sizeof(event));

		event.data.fd = client;
		event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
		if (epoll_ctl(this->_epollfd, EPOLL_CTL_ADD, client, &event) == -1)
			throw serverSocket::SocketException("Error: epoll_ctl\n");
		this->_request[index].insert(std::pair<int, std::vector<char> >(client, std::vector<char>()));
		this->_clientsfd[index].push_back(client);
		this->_readingStatus[index][client] = false;
	}
}

//Reading client's request
void serverSocket::read_client(epoll_event *events, int j, int index)
{
	char buffer[4096];
	int fd = events[j].data.fd;
	std::map<int, std::vector<char> >::iterator it = this->_request[index].find(fd);
	
	if (it == this->_request[index].end())
	{
		this->_request[index].insert(std::pair<int, std::vector<char> >(fd, std::vector<char>()));
		it = this->_request[index].find(fd);
	}
	
	memset(buffer, 0, 4096);
	ssize_t nbytes = read(events[j].data.fd, buffer, 4096);
	this->_lastActivity[index][events[j].data.fd] = time(NULL);

	if (nbytes < 0)
	{
		send_request(it->second, fd, false, index);
		return ;
	}	
	else if (nbytes > 0)
		it->second.insert(it->second.end(), buffer, buffer + nbytes);
	else
	{
		send_request(it->second, fd, false, index);
		std::cout << "Disconnecting" << std::endl;
		return ;
	}

	send_request(it->second, events[j].data.fd, true, index);
}


//////////////////// Utils //////////////////

//closing everyting relating to the handled client
void serverSocket::close_fd(int fd, int index)
{
	std::vector<int>::iterator itv = this->_clientsfd[index].begin();

	while (itv != this->_clientsfd[index].end() && *itv != fd)
		itv++;
	if (itv != this->_clientsfd[index].end())
		_clientsfd[index].erase(itv);
	epoll_ctl(this->get_epollfd(), EPOLL_CTL_DEL, fd, NULL);
	this->_request[index].erase(fd);
	this->_contentLength[index].erase(fd);
	this->_lastActivity[index].erase(fd);
	this->_readingStatus[index].erase(fd);
	close(fd);
}

//check if the fd is a server
bool serverSocket::check_server(int fd)
{
	for (std::vector<int>::iterator it = _serverfd.begin(); it != _serverfd.end(); it++)
	{
		if (fd == *it)
			return (true);
	}
	return (false);
}

//associate the server fd to the right index
int serverSocket::get_server_index_from_fd(int fd) const
{
	for (int i = 0; i < _nbServer; ++i)
	{
		if (std::find(_clientsfd[i].begin(), _clientsfd[i].end(), fd) != _clientsfd[i].end())
			return (i);
		if (fd == _serverfd[i])
			return (i);
	}
	return (-1);
}

//////////////////EXCEPTIONS//////////////////

const char *serverSocket::SocketException::what() const throw()
{
	return (this->_reason.c_str());
}
