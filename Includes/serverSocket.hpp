#pragma once

# include "webserv.hpp"
# include "httpRequest.hpp"
# include "Config.hpp"
# include "HandleCGI.hpp"

class serverSocket
{
	private:
		int													_nbServer;
		int													_epollfd;
		std::vector<int> 									_serverfd;
		std::vector<std::vector<int> >						_clientsfd;
		std::vector<std::map<int, time_t> >					_lastActivity;
		std::vector<struct sockaddr_in>						_serverAddress;
		std::vector<std::map<int, std::vector<char> > >		_request;
		std::vector<std::map<int, size_t> >					_contentLength;
		std::vector<std::map<int, bool> > 					_readingStatus;
		const std::vector<ServerParse>&						_serverConfig;
		std::vector<std::map<std::string, std::string> >	_cookies;
		std::map<int, HandleCGI*>							_cgi;
		
		void	execution_loop();
		void	server_loop(int index);
		bool	check_server(int fd);
		void	read_client(epoll_event *events, int i, int index);
		void 	send_request(std::vector<char> &request, int client_fd, bool connection_status, int index);
		void	responding_to_cgi(int fd);
		void	checkTimeout();
		
	public:
		serverSocket(Config& config);
		virtual ~serverSocket();
		
		//Getter and Setter
		int 									getServerfd(int index) const;
		int 									get_epollfd() const;
		struct sockaddr_in 						getServerAddress(int index) const;
		std::map<std::string, std::string> &	getCookies(int index);
		const std::vector<ServerParse>&			getServerConfig() const;
		void									setReadingStatus(bool status, int index, int fd);
		void									setCookies(std::map<std::string, std::string> & cookies, int index);
		
		void									initialize();
		void									close_fd(int fd, int index);
		int										get_server_index_from_fd(int fd) const;

	//Exceptions
		class SocketException : virtual public std::exception
		{
			private:
				const std::string _reason;

			public:
				SocketException(const std::string reason): _reason(reason) {}
				virtual const char *what() const throw();
				virtual ~SocketException() throw () {};
		};
};