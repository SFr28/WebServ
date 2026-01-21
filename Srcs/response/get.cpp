#include "httpResponse.hpp"

//Read a File and put its content in the body of the response
bool	httpResponse::foundBody()
{
	std::ifstream		file(this->_path.c_str(), std::ios::binary);
	std::string			buffer;
	std::ostringstream	body;

	if (!file)
		return false;
	while (std::getline(file, buffer))
		body << buffer << "\n";
	
	file.close();

	std::string	bodyStr = body.str();
	this->_body = std::vector<char>(bodyStr.begin(), bodyStr.end());

	return true;
}

//generate an html with the list of file in the requested directory
bool	httpResponse::generateAutoindex(std::string const & path)
{
	if (isAutoIndexAllowed(this->_location, *this->_server) == false)
		return false;

	std::string	body = "";
	std::string	uri = this->_request.getTarget();

	DIR*		dir = opendir(path.c_str());
	if (!dir)
		return false;

	body += "<html>\n<head><title>Index of ";
	body += uri;
	body += "</title></head>\n<body>";
	body += "\t<h1>Index of ";
	body += uri;
	body += "</h1>\n\t<ul>\n";

	std::string	name;
	struct dirent*	entry;
	while ((entry = readdir(dir)) != NULL)
	{
		name = entry->d_name;

		if (name == "." || name == "..")
			continue;
		
		body += "\t\t<li><a href=\"";
		body += uri;
		if (uri[uri.length() - 1] != '/')
			body += "/";
		body += name;
		body += "\">";
		body += name;
		body += "</a></li>\n";
	}

	body += "\t</ul>\n</body>\n</html>";
	this->_body = std::vector<char>(body.begin(), body.end());

	closedir(dir);

	return true;
}

//When the method requested is GET
bool	httpResponse::handleGET()
{
	if (this->_request.getBody().empty() == false)
	{
		this->_statusCode = BAD_REQUEST;
		return false;
	}
	
	if (isDirectory(this->_path) == true)
	{
		if (generateAutoindex(this->_path) == true)
		{
			this->_headerFields["Content-Type"] = "text/html";
			return true;
		}

		this->_statusCode = FORBIDDEN;
		return false;
	}

	if (foundBody() == false)
	{	
		this->_statusCode = NOT_FOUND;
		return false;
	}

	this->_headerFields["Content-Type"] = identifyMIME(this->_path);
	return true;
}
