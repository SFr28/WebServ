#include "webserv.hpp"
#include "Macro_CodeErrors.hpp"
#include "httpRequest.hpp"

//HTTP Request :
// METHOD url VERSION	(ex : GET /path/to/file.html HTTP/1.1)
// Header field			(ex :	Content-Type: text/plain)
//						(		Content-Length: 6		)
//						(		Connection: close		)
// "\r\n"
// Body					(ex : Hello!)

///////////////////////// Constructor/Destructor ///////////////////////
httpRequest::httpRequest(std::vector<char> &data, std::vector<ServerParse> servers) : _serversList(servers), _server(NULL), _printRequest(""), _method(""), _target(""), _headers(), _statusCode(200), _isParsed(true)
{
	if (parseRawData(data) == false)
		return;
	if (readFirstLine() == false)	
		return;
	if (readHeaderFields() == false)
		return;
	if (checkBody() == false)
		return;
}

httpRequest::httpRequest(int statusCode, std::vector<ServerParse> servers) : _serversList(servers), _server(NULL), _printRequest(""), _method(""), _target(""), _headers(), _statusCode(statusCode), _isParsed(true)
{}

httpRequest::~httpRequest(){}


//////////////////////////// Methods ////////////////////////////
//Separe the header part of the request and the body
bool	httpRequest::parseRawData(std::vector<char> &data)
{
	std::string line = "";
	bool		body = false;

	if (data.empty() == true)
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}

	size_t	len = 0;
	for (size_t i = 0; data[i]; i++)
	{
		line += data[i];
		len ++;
		if (data[i] == '\n')
		{
			this->_data.push_back(line);
			if (line == "\r\n" && body == false)
			{
				body = true;
				break;
			}
			line.clear();
		}
	}
	this->_printRequest.insert(this->_printRequest.end(), data.begin(), data.begin() + len);

	if (body == true && len != data.size())
		this->_body.insert(this->_body.end(), data.begin() + len, data.end());

	return true;
}

//----------------------- RequestLine ---------------------------------
//Check if the method is implemented
bool	httpRequest::checkMethod(std::string & method)
{
	if (method == "GET")
		return true;
	if (method == "DELETE")
		return true;
	if (method == "POST")
		return true;
	return false;
}

//Get the method from the first line of the request
int	httpRequest::readMethod(std::string &firstLine)
{
	std::string	method;
	size_t		pos;

	pos = firstLine.find(' ');
	if (pos == std::string::npos || pos == 0)
	{
		this->_statusCode = BAD_REQUEST;
		return BAD_REQUEST;
	}

	method = firstLine.substr(0, pos);
	this->_method = method;
	if (checkMethod(method) == false)
	{
		this->_statusCode = NOT_IMPLEMENTED;
		return NOT_IMPLEMENTED;
	}
	return 0;
}

//Get the URI from the first line of the request
int	httpRequest::readTarget(std::string &firstLine)
{
	std::string	uri;
	size_t		pos, pos2;

	pos = firstLine.find(' ');
	pos2 = firstLine.find_last_of(' ');
	if (pos == std::string::npos || pos == 0)
	{
		this->_statusCode = BAD_REQUEST;
		return BAD_REQUEST;
	}
	if (pos == pos2)
	{
		this->_statusCode = BAD_REQUEST;
		return BAD_REQUEST;
	}
	this->_target = firstLine.substr(pos + 1, pos2 - pos - 1);
	if (this->_target[this->_target.size() - 1] == '?')
		this->_target = this->_target.substr(0, this->_target.size() - 1);
	if (this->_target.length() > MAX_URI_LENGTH)
	{
		this->_statusCode = URI_TOO_LONG;
		return URI_TOO_LONG;
	}
	if (this->_target.find(' ') != std::string::npos)
	{
		this->_statusCode = BAD_REQUEST;
		return BAD_REQUEST;
	}
	return 0;
}

//Get the http version from the first line
int	httpRequest::readVersion(std::string &firstLine)
{
	size_t		pos;

	pos = firstLine.find_last_of(' ');
	this->_version = firstLine.substr(pos + 1, 8);
	if (this->_version != "HTTP/1.1")
	{
		this->_statusCode = HTTP_VERSION_NOT_SUPPORTED;
		return HTTP_VERSION_NOT_SUPPORTED;
	}
	return 0;
}

//Parse the first line of the request
bool	httpRequest::readFirstLine()
{
	size_t		pos;
	std::string firstLine;

	if (!this->_data.empty())
		firstLine = this->_data[0];
	else
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}

	if (firstLine.empty())
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
		
	pos = firstLine.find("\r\n");
	
	if (pos == std::string::npos || pos == 0)
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
		
	firstLine = firstLine.substr(0, pos + 1);

	if (readMethod(firstLine) != 0)
		return false;
	
	if (readTarget(firstLine) != 0)
		return false;
	
	if (readVersion(firstLine) != 0)
		return false;
	return true;
}

//----------------------- Header Fields ---------------------------------
//Check if the Content-Length header is present in the header
//If the method is "GET", check if it's absent or equal to 0
bool	httpRequest::checkForContentLength()
{
	std::map<std::string, std::string>::iterator	it;
	
	it = this->_headers.find("content-length");

	if (this->_method == "GET")
	{
		if (it != this->_headers.end() && it->second.empty() == false && std::atoi((it->second).c_str()) != 0)
		{
			this->_statusCode = BAD_REQUEST;
			return false;
		}
		return true;
	}

	if (it == this->_headers.end() || it->second.empty())
	{
		if (this->_body.empty())
			return true;
		this->_statusCode = LENGTH_REQUIRED;
		return false;
	}
	
	this->_contentLength = std::atoi((it->second).c_str());

	return true;
}

//Check if the Content-Type header is present in the header
bool	httpRequest::checkForContentType()
{
	std::map<std::string, std::string>::iterator	it;
	
	it = this->_headers.find("content-type");
	if (it == this->_headers.end())
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
		
	this->_contentType = it->second;
	return true;
}

//Check if the Host is correct based on the host of the server
bool	httpRequest::isHostValid(std::string &host)
{
	std::string	ip;
	int			port;
	std::string	ipServ;
	int			portServ;
	size_t		colon = host.find(':');
	
	if (colon == std::string::npos)
		return false;

	ip = host.substr(0, colon);
	port = std::atoi(host.substr(colon + 1).c_str());
	
	if (ip == "localhost")
		ip = "127.0.0.1";
	
	for (std::vector<ServerParse>::iterator it = this->_serversList.begin(); it < this->_serversList.end(); it++)
	{
		ipServ = it->getHost();
		portServ = std::atoi(it->getListen().c_str());

		if (ipServ == ip && portServ == port)
		{
			this->_server = &(*it);
			return true;
		}
	}

	return false;
}

//Check if the Host header is present in the header
bool	httpRequest::checkForHost()
{
	if (this->_headers.find("host") == this->_headers.end())
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}

	if (isHostValid(this->_headers["host"]) == false)
	{
		this->_statusCode = BAD_REQUEST;
		return false;	
	}
	
	return true;
}

//Parse header fields
bool	httpRequest::readHeaderFields()
{
	size_t		pos_colon, pos_line;
	std::string	key, value;
	std::pair<std::map<std::string, std::string>::iterator, bool>	pair;

	std::vector<std::string>::iterator	it = this->_data.begin() + 1;
	while (it != this->_data.end() && *it != "\r\n")
	{
		pos_line = (*it).find('\n');
		pos_colon = (*it).find(':');

		if (pos_colon == std::string::npos || pos_colon == 0)
		{
			this->_statusCode = BAD_REQUEST;
			return false;
		}
		key = (*it).substr(0, pos_colon);
		if (key.find_first_of(" \t") != std::string::npos)
		{
			this->_statusCode = BAD_REQUEST;
			return false;
		}
	
		value = (*it).substr(pos_colon + 2, pos_line - pos_colon - 2);
	
		pair = this->_headers.insert(std::pair<std::string, std::string>(toLower_string(key), value));
		if (pair.second == false)
		{
			this->_statusCode = BAD_REQUEST;
			return false;
		}
		it++;
	}
	
	if (it == this->_data.end())
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
	
	if (checkForHost() == false)
		return false;
	
	return true;
}

//----------------------------- Body ---------------------------------
//Check the body of the request
//If the body size is different from Content-Length ->set a BAD REQUEST
//If its inferior -> specify that the request is not compelte -> return in read loop
bool	httpRequest::checkBody()
{	
	if (checkForContentLength() == false)
		return false;

	if (this->_body.empty())
		return false;
	
	if (checkForContentType() == false)
		return false;

	int	len = this->_body.size();

	if (len > std::atoi(this->_server->getSize().c_str()))
	{
		this->_statusCode = CONTENT_TOO_LARGE;
		return false;
	}

	if (len < this->_contentLength)
	{
		this->_statusCode = BAD_REQUEST;
		this->_isParsed = false;
		return false;
	}
	if (len > this->_contentLength)
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
	
	return true;
}

///////////////////////////// Getters ////////////////////////////
std::vector<std::string> const &	httpRequest::getData() const
{
	return (this->_data);
}

std::string const &	httpRequest::getprintRequest() const
{
	return (this->_printRequest);
}

std::string const& httpRequest::getMethod() const
{
	return (this->_method);
}

std::string const & httpRequest::getTarget() const
{
	return (this->_target);
}

std::string const & httpRequest::getVersion() const
{
	return (this->_version);
}

int	const &	httpRequest::getStatusCode() const
{
	return (this->_statusCode);
}

std::map<std::string, std::string> const &	httpRequest::getHeader() const
{
	return (this->_headers);
}

std::vector<char> const &	httpRequest::getBody() const
{
	return (this->_body);
}

int const &	httpRequest::getContentLength() const
{
	return (this->_contentLength);
}

std::string const &	httpRequest::getContentType() const
{
	return (this->_contentType);
}

bool httpRequest::getIsParsed() const
{
	return (this->_isParsed);
}

const ServerParse&	httpRequest::getServer() const
{
	return (*this->_server);
}
