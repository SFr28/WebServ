#include "httpResponse.hpp"

////////////////////////////// CONSTRUCTOR/DESTRUCTOR //////////////////////

httpResponse::httpResponse(httpRequest&	request, std::map<std::string, std::string> & cookies) :	_server(&request.getServer()),
																									_request(request),
																									_statusCode(request.getStatusCode()), 
																									_reasonPhrase(""),
																									_delete(false),
																									_cookies(cookies),
																									_location(NULL),
																									_isCGI(false)

{
	makeCookie();
	if (this->_statusCode != 200)
	{
		handleError();
		return ;
	}
	if (checkPath(this->_request.getTarget()) == false)
	{
		handleError();
		return ;
	}
	if (checkMethod(this->_request.getMethod()) == false)
	{
		handleError();
		return ;
	}
	if (checkCGI() == true)
	{
		this->_isCGI = true;
		return;
	}

	handleMethod();
}

httpResponse::~httpResponse()
{}

/////////////////////////////// PATH FUNCTIONS ////////////////////////////

//If the path requested is a directory and not a file
//Check if there are an index file in this directory
//If not check if the autoindex is on 
bool	httpResponse::handleDirectory(std::string & path)
{
	std::string	indexPath = path;

	if (indexPath[indexPath.length() - 1] != '/')
		indexPath += "/";
	
	indexPath += this->_server->getIndex();

	if (pathExists(indexPath))
	{
		this->_path = indexPath;
		return true;
	}

	if (isAutoIndexAllowed(this->_location, *this->_server) == true)
	{
		this->_path = path;
		return true;
	}

	this->_statusCode = FORBIDDEN;

	return false;
}

/////////////////////////////// CHECK /////////////////////////////////////
//Compare the method requested to the list of allowed method by the server
//If it's not -> 405 Method Not Allowed
bool	httpResponse::checkMethod(std::string const & method)
{
	std::vector<std::string>	allowedMethod;

	if (this->_location != NULL && this->_location->getMethod().empty() == false)
		allowedMethod = this->_location->getMethod();
	else
		return true;

	std::vector<std::string>::iterator	it;

	for (it = allowedMethod.begin(); it < allowedMethod.end(); it++)
	{
		if (*it == method)
			return true;
	}

	this->_statusCode = METHOD_NOT_ALLOWED;

	return false;
}

//Compare the uri to all the possible location of the server
//Set the longest match has the location of the server.
//Construct the path based on the root of the location
//or the default root of the server if no location or no root for
//this location
std::string	httpResponse::checkLocation(std::string const & uri)
{
	std::vector<LocationParse*>				locationVec = this->_server->getLocation();
	std::vector<LocationParse*>::iterator	it;
	size_t									longestLength = 0;
	LocationParse*							matchedLocation = NULL;

	for (it = locationVec.begin(); it != locationVec.end(); ++it)
	{
		std::string locName = (*it)->getName();

		if (uri.compare(0, locName.length(), locName) == 0 && locName.length() > longestLength)
		{
			longestLength = locName.length();
			matchedLocation = *it;
		}
	}

	std::string path;

	if (matchedLocation != NULL && !matchedLocation->getReturnStatus().empty())
		this->_statusCode = std::atoi(matchedLocation->getReturnStatus().c_str());
	
	if (matchedLocation != NULL && (!matchedLocation->getReturnPath().empty() || !matchedLocation->getRoot().empty()))
	{
		this->_location = matchedLocation;

		if (!matchedLocation->getReturnPath().empty())
			return matchedLocation->getReturnPath();

		std::string remainingUri = uri.substr(longestLength);
		path = matchedLocation->getRoot();
		if (remainingUri.empty() == false && path[path.size() - 1] != '/')
			path += "/";
		path += remainingUri;
	}

	else
	{
		path = this->_server->getRoot();
		path += uri;
		this->_location = NULL;
	}

	return path;
}

//Extract the filename from a JSON body
static std::string extractFilenameFromJSON(std::vector<char> const & jsonBody) 
{
	std::string	tmp(jsonBody.begin(), jsonBody.end());
    std::string key = "\"filename\":\"";
	size_t start = tmp.find(key);

    if (start == std::string::npos) 
		return "";

    start += key.length();
    size_t end = tmp.find("\"", start);

    if (end == std::string::npos)
		return "";
	
    return tmp.substr(start, end - start);
}

//Check and construct the URI requested in the request
//Check that the URI is not empty and begin by '/' -> if not 400 BAD REQUEST
//Add the root of the server to the path by checking the location
//If a MOVED_PERMANENTLY have been placed in statusCode (redirect) -> set the path has a header and return
//Check if there is a query(?) in the URI -> if yes, extract the path (all before the ?)
//If the method is delete and the path is a directory -> extract the filename to delete from the body of the request
//Security protection : check if there is ".." in the path -> if yes, possible securitissues
//the request could try to get data from outside the root of the server
//=> To prevent that : 403 Forbidden
//Check if the path exist -> 404 Not Found if not
//If it's a directory and not a DELETE method -> handle directory
bool	httpResponse::checkPath(std::string const & uri)
{
	if (uri.empty() || uri[0] != '/')
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
	
	std::string	path = checkLocation(uri);

	if (this->_statusCode == MOVED_PERMANENTLY)
	{
		this->_headerFields["Location"] = path;
		return false;
	}

	size_t	pos = path.find('?');

	if (pos != std::string::npos)
		path = path.substr(0, pos);

	if (this->_request.getMethod() == "DELETE" && isDirectory(path) == true)
	{
		std::string	filename = extractFilenameFromJSON(this->_request.getBody());
		this->_delete = true;
		if (filename.empty() == true)
		{
			this->_statusCode = BAD_REQUEST;
			return false;
		}
		
		if (path[path.size() - 1] != '/')
			path += "/";

		path += filename;
	}
	
	if (path.find("..") != std::string::npos)
	{
		this->_statusCode = FORBIDDEN;
		return false;
	}

	if (pathExists(path) == false)
	{
		this->_statusCode = NOT_FOUND;
		return false;
	}

	if (isDirectory(path) == true && this->_request.getMethod() != "DELETE")
		return handleDirectory(path);
	
	this->_path = path;

	return true;
}

//Check if the file requested is a CGI or not
//Compare the location to the possible location of CGI (from config file)
//Compare the extension of the file to the possible extension of CGI
bool	httpResponse::checkCGI()
{
	if (this->_path.find("cgi-bin") == std::string::npos)
		return false;
	
	size_t		pos = this->_path.find_last_of('.');
	
	if (pos == std::string::npos)
		return false;
	
	std::string	extension = this->_path.substr(pos);
	std::vector<std::string>	allowedExt;

	allowedExt.push_back(".pl");
	allowedExt.push_back(".py");
	
	if (std::find(allowedExt.begin(), allowedExt.end(), extension) != allowedExt.end())
	{
		this->_CGI = true;
		return true;
  }
	return false;
}

////////////////////////////// METHODS ////////////////////////////////////

//Handle the request depending of the method type
void	httpResponse::handleMethod()
{
	std::string	method = this->_request.getMethod();
	bool		noError = true;

	if (this->_statusCode < 400)
	{
		if (method == "GET")
			noError = handleGET();
		if (method == "POST")
			noError = handlePOST();
		if (method == "DELETE")
			noError = handleDELETE();
	}
	if (noError == false || this->_statusCode >= 400)
		handleError();
}

//When the method requested is DELETE
bool	httpResponse::handleDELETE()
{
	if (isDirectory(this->_path) == true)
	{
		this->_statusCode = FORBIDDEN;
		return false;
	}

	if (std::remove(this->_path.c_str()) != 0)
	{
		this->_statusCode = FORBIDDEN;
		return false;
	}

	this->_statusCode = NO_CONTENT;

	return true;
}

//When an error as been detected, redirect to the appropriate error page from the
//config file, if it does not exist, or it's a delete error, generate a page with
//the status code and the reason in plain text
void	httpResponse::handleError()
{
	if (this->_server && this->_server->getPage().empty() == false)
	{
		std::map<int, std::string>	errorPage = this->_server->getPage();

		if (this->_delete == false && errorPage.find(this->_statusCode) != errorPage.end())
		{
			this->_path = errorPage[this->_statusCode];
			foundBody();
			this->_headerFields["Content-Type"] = identifyMIME(this->_path);
			return;
		}
	}

	std::string	body = toStr(this->_statusCode);

	body += " ";
	body += identifyReasonPhrase(this->_statusCode);

	this->_body = std::vector<char>(body.begin(), body.end());
	this->_headerFields["Content-Type"] = "text/plain";
}

void httpResponse::handleCGIError()
{
	this->_path = "www/500.html";
	if (pathExists(this->_path) && isDirectory(this->_path) == false)
    {
		foundBody();
		this->_headerFields["Content-Type"] = identifyMIME(this->_path);
		return;
	}
	std::string	body = toStr(this->_statusCode);

	body += " ";
	body += identifyReasonPhrase(this->_statusCode);

	this->_body = std::vector<char>(body.begin(), body.end());
	this->_headerFields["Content-Type"] = "text/plain";
}

///////////////////////////////////////////////////////////////////////////

static std::string getHeader(std::vector<char>& buffer)
{
	std::string header;
	size_t i = 0;

	while (i < buffer.size() && buffer[i] != '\n') 
	{
		header += buffer[i];
		i++;
	}
	
	if (i < buffer.size())
		i++;
	buffer.erase(buffer.begin(), buffer.begin() + i);
	
	size_t colon = header.find(':');
	if (colon != std::string::npos)
		return header.substr(colon + 1);
	return "text/plain";
}

void	httpResponse::finalizeCGIResponse(std::vector<char>& buffer)
{
	this->_headerFields["content-type"] = getHeader(buffer);
	this->_body = buffer;
	this->_body.insert(this->_body.end(), '\n');
}

//Function that construct the complete http response to send back to the 
//client who made the request
void	httpResponse::generateResponse()
{
	std::ostringstream	response;
	this->_reasonPhrase = identifyReasonPhrase(this->_statusCode);

	response << "HTTP/1.1" << " ";
	response << this->_statusCode << " ";
	response << this->_reasonPhrase << "\r\n";

	this->_headerFields["Connection"] = "keep-alive";
	this->_headerFields["Content-Length"] = toStr(this->_body.size());
	this->_headerFields["Date"] = generateDate();
	for(std::map<std::string, std::string>::iterator it = this->_headerFields.begin(); it != this->_headerFields.end(); it++)
		response << it->first << ": " << it->second << "\r\n";
	
	response << "\r\n";
	std::string	responseStr = response.str();
	this->_response = std::vector<char>(responseStr.begin(), responseStr.end());
	if (this->_body.empty() == false)
	this->_response.insert(this->_response.end(), this->_body.begin(), this->_body.end());
	this->_lenResponse = this->_response.size();
}

////////////////////////////// SETTERS / GETTERS ////////////////////////////
std::vector<char> const &	httpResponse::getResponse(void) const
{
	return (this->_response);
}

size_t const &	httpResponse::getLenResponse(void) const
{
	return (this->_lenResponse);
}

std::map<std::string, std::string> &	httpResponse::getCookies()
{
	return (this->_cookies);
}

bool const &	httpResponse::getIsCGI() const
{
	return (this->_isCGI);
}

std::string &	httpResponse::getPath()
{
	return (this->_path);
}

ServerParse const &		httpResponse::getServer() const
{
	return (*this->_server);
}

int	httpResponse::getStatusCode() const
{
	return (this->_statusCode);
}

void	httpResponse::setStatusCode(int statusCode)
{
	this->_statusCode = statusCode;
}