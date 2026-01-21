#pragma once

# include "webserv.hpp"

class HandleCGI
{
	private:
		const ServerParse& 						_server;
		httpRequest&							_request;
		std::map<std::string, std::string>		_env;
		std::vector<char>*						_bodyResponse;
		char**									_envInChar;
		std::string								_path;
		std::string								_file_name;
		std::FILE*								_tmp_in;
		int										_fd_tmp_in;
		int 									_client_fd; 
    	httpResponse* 							_response;
    	int 									_pipe_fd[2];
    	pid_t 									_pid;
		pid_t									_w;
		int										_execStatus;
	
		void 									read_cgi_response();
		void									execute_cgi();
		void									initialize_env();
		void									env_in_char();
		int										create_tmp_files();
	
	public:
		HandleCGI(httpRequest &request, const ServerParse& server, httpResponse *response, int clientfd);
		~HandleCGI();
		
		//Setter and Getter
		std::vector<char>&	getBodyResponse() const;
		httpResponse*		getResponse();
		std::string			getHeader();
		int					getFdOut() const;
		int					getClientFd() const;
		pid_t				getPID();
		void				setw(pid_t w);
		void				setExecStatus(int status);
	
		//Methods
		int 				parent_process();
		void				startCGI();
};