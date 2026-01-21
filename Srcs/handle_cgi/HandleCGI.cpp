#include "httpResponse.hpp"
#include "HandleCGI.hpp"
#include "Macro_CodeErrors.hpp"

///////////////////////// Constructor/Destructor ///////////////////////

HandleCGI::HandleCGI(httpRequest &request, const ServerParse& server, httpResponse *response, int clientfd): _server(server), _request(request), _path(response->getPath()), _client_fd(clientfd), _response(response)
{
	this->_bodyResponse = new std::vector<char>();
	initialize_env();
}

HandleCGI::~HandleCGI()
{
	if (this->_envInChar)
	{
		for (int i = 0; this->_envInChar[i]; i++)
			delete[] (this->_envInChar[i]);
		delete[] (this->_envInChar);
	}	
	delete this->_bodyResponse;
}

std::vector<char>&	HandleCGI::getBodyResponse() const
{
	return (*this->_bodyResponse);
}

httpResponse*	HandleCGI::getResponse()
{
	return (this->_response);
}

int	HandleCGI::getFdOut() const
{
	return (this->_pipe_fd[0]);
}

int	HandleCGI::getClientFd() const
{
	return (this->_client_fd);
}

std::string HandleCGI::getHeader()
{
	std::string header;
	std::vector<char>	tmp = *this->_bodyResponse;
	size_t i = 0;

	while (i < tmp.size() && tmp[i] != '\n') 
	{
		header += tmp[i];
		i++;
	}
	
	if (i < tmp.size())
		i++;
	tmp.erase(tmp.begin(), tmp.begin() + i);
	
	size_t colon = header.find(':');
	if (colon != std::string::npos)
		return header.substr(colon + 1);
	return "text/plain";
}

void	HandleCGI::setw(pid_t w)
{
	this->_w = w;
}
void	HandleCGI::setExecStatus(int status)
{
	this->_execStatus = status;
}

pid_t HandleCGI::getPID()
{
	return (this->_pid);
}

///////////////////////// Environment ///////////////////////

void HandleCGI::initialize_env()
{
	std::map<std::string, std::string>	header = this->_request.getHeader();
	
	size_t pos = this->_path.find_last_of('/');
	if (pos != std::string::npos)
		this->_file_name = this->_path.substr(pos + 1);
	else
		this->_file_name = "";

	pos = this->_request.getTarget().find('?');
	std::string query;
	if (pos != std::string::npos)
		query = this->_request.getTarget().substr(pos);
	else
		query = "";
	
	if (this->_request.getMethod() == "POST")
	{
		std::string content_length = toStr((size_t)this->_request.getContentLength());
		this->_env.insert(std::pair<std::string, std::string>("CONTENT_LENGTH", content_length));
		this->_env.insert(std::pair<std::string, std::string>("CONTENT_TYPE", this->_request.getContentType()));
	}
	this->_env.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
	this->_env.insert(std::pair<std::string, std::string>("REQUEST_URI", this->_request.getTarget()));
	this->_env.insert(std::pair<std::string, std::string>("PATH_INFO", this->_path));
	this->_env.insert(std::pair<std::string, std::string>("PATH_TRANSLATED", this->_path));
	this->_env.insert(std::pair<std::string, std::string>("QUERY_STRING", query));
	this->_env.insert(std::pair<std::string, std::string>("REQUEST_METHOD", this->_request.getMethod()));
	this->_env.insert(std::pair<std::string, std::string>("SCRIPT_FILENAME", _file_name));
	this->_env.insert(std::pair<std::string, std::string>("SCRIPT_NAME", _file_name));
	this->_env.insert(std::pair<std::string, std::string>("SERVER_PROTOCOL", "HTTP/1.1"));
	this->_env.insert(std::pair<std::string, std::string>("SERVER_PORT", this->_server.getListen()));
}

void HandleCGI::env_in_char()
{
	size_t size = this->_env.size() + 1;
	this->_envInChar = new char*[size];
	if (this->_envInChar == NULL)
		throw std::bad_alloc();
	std::map<std::string, std::string>::iterator	it;
	std::string	param;
	size_t i = 0;

	for(it = this->_env.begin(); it != this->_env.end(); it++)
	{
		param = it->first + "=" + it->second;
		this->_envInChar[i] = new char[param.size() + 1];
		if (this->_envInChar[i] == NULL)
			throw std::bad_alloc();
		std::strcpy(this->_envInChar[i], param.c_str());
		i++;
		param.clear();
	}
	this->_envInChar[size - 1] = NULL;
}

///////////////////////// CGI execution ///////////////////////

//Creating tmp files to hold the body of the request and to write the response to
int	HandleCGI::create_tmp_files()
{
	if (pipe(this->_pipe_fd) == -1)
	{
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
		return (INTERNAL_SERVER_ERROR);
	}
	this->_tmp_in = std::tmpfile();
	if (!this->_tmp_in)
		return (-1);
	this->_fd_tmp_in = fileno(this->_tmp_in);

	if (this->_request.getMethod() == "POST")
	{
		ssize_t body_size = _request.getBody().size();
		std::string body(_request.getBody().data(), body_size);
		if (body_size > 0)
			std::fwrite(body.c_str(), sizeof(char), body_size, this->_tmp_in);
		std::rewind(this->_tmp_in);
	}
	return (0);
}

//reading the cgi response after execution and storing it
void HandleCGI::read_cgi_response()
{
	char buffer[1024];
	
	memset(buffer, 0, 1024);
	FILE* file = fdopen(this->_pipe_fd[0], "r");
	size_t r;
	while ( (r = std::fread(buffer, sizeof(char), sizeof(buffer), file)) > 0)
	{
		this->_bodyResponse->insert(this->_bodyResponse->end(), buffer, buffer + r);
		memset(buffer, 0, 1024);
	}
	std::fclose(file);
	close(this->_pipe_fd[0]);
}

//we're in the parent process, we check if the cgi was executed without problem and we store the response
int HandleCGI::parent_process()
{
	if (this->_w == -1)
	{
		close(this->_pipe_fd[0]);
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
		return (INTERNAL_SERVER_ERROR);
	}
	if (WIFEXITED(this->_execStatus))
	{
		if (WEXITSTATUS(this->_execStatus) != 0)
		{
			close(this->_pipe_fd[0]);
			this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
			return (INTERNAL_SERVER_ERROR);
		}
	}
	else if (WIFSIGNALED(this->_execStatus))
	{
		close(this->_pipe_fd[0]);
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
		return (INTERNAL_SERVER_ERROR);
	}
	read_cgi_response();
	return OK;
}

//pid == 0: in child process, execve cgi script, if it crashed, exit
//pid > 0 :in parent process, get what the cgi script wrote on fdout => it's the response
//         handle timeout (5min) with waitpid => if timeout > 10s, kill process, close and return 504 
//pid < 0: fork crashed, close and return 500 (error)
void HandleCGI::execute_cgi()
{
	std::string response;
	char 		*argv[2];

	if (access(this->_path.c_str(), X_OK | R_OK) == -1)
	{
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
		return;
	}

	argv[0] = new char[_file_name.length() + 1];
	if (argv[0] == NULL)
		throw std::bad_alloc();
	argv[0] = strcpy(argv[0], this->_file_name.c_str());
	argv[1] = NULL;

	create_tmp_files();

	this->_pid = fork();
	if (this->_pid == 0)
	{
		if (dup2(this->_pipe_fd[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "dup2 fd[1] failed" << std::endl;
			exit(1);
		}
		close (this->_pipe_fd[1]);
		if (dup2(this->_fd_tmp_in, STDIN_FILENO) == -1)
		{
			std::cerr << "dup2 fd[0] failed" << std::endl;
			exit(1);
		}
		close(this->_pipe_fd[0]);
		fclose(this->_tmp_in);
		execve(this->_path.c_str(), argv, this->_envInChar);
		std::cerr << "execve crashed" << std::endl;
		delete[] argv[0];
		exit(1);
	}
	else if (this->_pid > 0)
	{
		fclose(this->_tmp_in);
		close(this->_pipe_fd[1]);
		delete[] argv[0];
		return;
	}
	else
	{
		fclose(this->_tmp_in);
		close(this->_pipe_fd[1]);
		close(this->_pipe_fd[0]);
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
		delete[] argv[0];
		return;
	}
}

void	HandleCGI::startCGI()
{
	try
	{
		env_in_char();
		execute_cgi();
	}
	catch (const std::bad_alloc &e)
	{
		std::cerr << RED << e.what() << RESET << std::endl;
		this->_response->setStatusCode(INTERNAL_SERVER_ERROR);
	}
}
