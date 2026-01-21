#include "ServerParse.hpp"

///////////CONSTRUCTOR / DESTRUCTOR///////////

ServerParse::ServerParse()
{
	this->_inServer = false;
	this->_listen = "";
	this->_host = "";
	this->_name = "";
	this->_root = "";
	this->_autoindex = "";
	this->_index = "";
	this->_client_max_body_size = "";
}

ServerParse::ServerParse(ServerParse const & copy)
{
	*this = copy;
}

ServerParse& ServerParse::operator=(ServerParse const & copy)
{
	if (this != &copy)
	{
		this->_inServer = copy._inServer;
        this->_listen = copy._listen;
        this->_host = copy._host;
		this->_name = copy._name;
        this->_root = copy._root;
        this->_autoindex = copy._autoindex;
        this->_index = copy._index;
        this->_client_max_body_size = copy._client_max_body_size;
		this->_error_page = copy._error_page;

		for (size_t i = 0; i < this->_location.size(); ++i)
            delete this->_location[i];
        this->_location.clear();

        for (size_t i = 0; i < copy._location.size(); ++i)
        {
            LocationParse* copiedLoc = new LocationParse(*(copy._location[i]));
            this->_location.push_back(copiedLoc);
        }
	}
	return *this;
}

ServerParse::~ServerParse()
{
	for (size_t i = 0; i < this->_location.size(); ++i)
        delete this->_location[i];
}

////////////////////SERVER////////////////////

void ServerParse::parseServer(std::ifstream& file, std::string line)
{
	this->_inServer = true;
	while (getline(file, line))
	{
		if (line.empty())
			continue;
		if (line.find("}") != std::string::npos)
		{
			this->_inServer = false;
			break ;
		}
		if (line.find("#") != std::string::npos)
			continue ;
		if (line.find("location") != std::string::npos)
		{
			LocationParse* loc = new LocationParse();
			loc->parseLocation(file, line);
			this->_location.push_back(loc);
			continue;
		}
		if (line[line.length() - 1] == ';')
			line.erase(line.length() - 1);
		std::string	state[8] = {"listen", "host", "name", "root", "autoindex", "index", "client_max_body_size", "error_page"};
		
		int i = 0;
		while (i < 8)
		{
			if (line.find(state[i]) != std::string::npos)
				break ;
			i++;
		}
		switch (i)
		{
			case 0:
				parsePort(line);
				break ;
			case 1:
				parseHost(line);
				break ;
			case 2:
				parseName(line);
				break ;
			case 3:
				parseRoot(line);
				break ;
			case 4:
				parseAuto(line);
				break ;
			case 5:
				parseIndex(line);
				break ;
			case 6:
				parseSize(line);
				break ;
			case 7:
				parsePage(line);
				break ;
			default:
				break;
		}
	}
}

std::vector<std::string>	ServerParse::splitTokens(const char* str)
{
	std::vector<std::string> tokens;

    char* token = std::strtok((char *)str, " \n\t\r\v\f");
    while (token != NULL) {
        tokens.push_back(token);
        token = std::strtok(NULL, " \n\t\r\v\f");
    }
    return tokens;
}

/////////////////////PORT/////////////////////

void	ServerParse::parsePort(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Listen: ";
		throw Config::NotEnoughArguments();
	}
    std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] Listen: ";
		throw Config::NotEnoughArguments();
	}
    std::string	port = token[1];
	if (isdigit(port[0]) && (atoi(port.c_str()) > 0 && atoi(port.c_str()) <= 65535))
		this->_listen = port;	
	else
	{
		std::cerr << CYAN << "[Server] Listen: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////HOST/////////////////////

bool	isValidIp(std::string &host)
{
	if (host == "localhost")
		return (true);
		
	std::istringstream ss(host);
    std::string token;
    int parts = 0;
    while (std::getline(ss, token, '.')) 
	{
        ++parts;
        if (parts > 4) 
			return (false);
        if (token.empty() || token.size() > 3) 
			return (false);
        for (std::string::size_type i = 0; i < token.size(); ++i) 
		{
            if (!std::isdigit(token[i]))
				return (false);
        }
        if (token.size() > 1 && token[0] == '0')
			return (false);
        int nb = std::atoi(token.c_str());
        if (nb < 0 || nb > 255)
			return (false);
    }
	return (true);
}

void	ServerParse::parseHost(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Host: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] Host: ";
		throw Config::NotEnoughArguments();
	}
	std::string	host = token[1];
	if (isValidIp(host) == true)
	{
		if (host == "localhost")
			this->_host = "127.0.0.1";
		else
			this->_host = host;
	}
		
    else
	{
		std::cerr << CYAN << "[Server] Host: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////NAME/////////////////////

void	ServerParse::parseName(std::string line)
{
	if (line.empty())
		this->_name = "Webserv";
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
	if (token.size() > 1)
		this->_name = token[1];
}

/////////////////////ROOT/////////////////////

void	ServerParse::parseRoot(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Root: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] Root: ";
		throw Config::NotEnoughArguments();
	}
	std::string	root = token[1];
	
	struct stat sb;
	if (stat(root.c_str(), &sb) == 0)
		this->_root = root;
	else
	{
		std::cerr << CYAN << "[Server] Root: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////INDEX////////////////////

void	ServerParse::parseIndex(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Index: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] Index: ";
		throw Config::NotEnoughArguments();
	}
	std::string	index = token[1];
	
	struct stat sb;
	std::string tmp;

	tmp = this->_root;
    tmp += "/";
    tmp += index;
	if (stat(tmp.c_str(), &sb) == 0)
		this->_index = index;
	else
	{
		std::cerr << CYAN << "[Server] Index: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////AUTO/////////////////////

void	ServerParse::parseAuto(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Autoindex: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] Autoindex: ";
		throw Config::NotEnoughArguments();
	}
	std::string	autoindex = token[1];
	if (autoindex == "on" || autoindex == "off")
		this->_autoindex = autoindex;
	else
	{
		std::cerr << CYAN << "[Server] Autoindex: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////SIZE/////////////////////

std::vector<std::string>	splitSize(const char* str, std::string sep)
{
	std::vector<std::string> tokens;

    char* token = std::strtok((char *)str, sep.c_str());
    while (token != NULL) {
        tokens.push_back(token);
        token = std::strtok(NULL, sep.c_str());
    }
    return tokens;
}

bool	isNbr(std::string token)
{
	if (token.empty())
		return (false);
	for (std::string::iterator it = token.begin(); it != token.end(); ++it)
	{
		if (!isdigit(*it))
			return (false);
	}
	return (true);
}

bool	isInRange(std::string& size)
{
	std::vector<std::string> token;

	if (size.find("m") != std::string::npos)
	{
		token = splitSize(size.c_str(), "m");
		if (isNbr(token[0]) == true && token.size() == 1)
		{
			for (int i = 0; i < 4; i++)
				token[0].push_back('0');
			size = token[0];
		}
		else
			return (false);
	}
	else if (size.find("M") != std::string::npos)
	{
		token = splitSize(size.c_str(), "M");
		if (isNbr(token[0]) == true && token.size() == 1)
		{
			for (int i = 0; i < 6; i++)
				token[0].push_back('0');
			size = token[0];
		}
		else
			return (false);
	}
	if (isNbr(size) == false)
		return (false);
	return (true);
}

void	ServerParse::parseSize(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] client_max_body_size: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {	
		std::cerr << CYAN << "[Server] client_max_body_size: ";
		throw Config::NotEnoughArguments();
	}
	std::string	size = token[1];
	if (isInRange(size) == true)
		this->_client_max_body_size = size;
	else
	{
		std::cerr << CYAN << "[Server] client_max_body_size: ";
		throw Config::InvalidArgument();
	}
}

/////////////////////ERROR////////////////////

void	ServerParse::parsePage(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Server] Error_Page: ";
		throw Config::NotEnoughArguments();
	}
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
	if (token.size() < 3)
	{	
		std::cerr << CYAN << "[Server] Error_Page: ";
		throw Config::NotEnoughArguments();
	}
	std::string path = token[token.size() - 1];

	for (size_t i = 1; i < token.size() - 1; i++)
	{
		if (isNbr(token[i]) == true && isNbr(path) == false)
		{
			int code = atoi(token[i].c_str());
			if (code < 400 || code > 599)
			{
				std::cerr << "[Server] code error incorrect" << std::endl;
				throw Config::InvalidArgument();
			}
				struct stat sb;
				if (stat(path.c_str(), &sb) == 0)
					this->_error_page[code] = path;
		}
		else
		{
			std::cerr << CYAN << "[Server] Error_Page: ";
			throw Config::InvalidArgument();
		}
	}
}

////////////////////GETTER////////////////////

std::string	ServerParse::getListen() const
{
	return (this->_listen);
}

std::string	ServerParse::getHost() const
{
	return (this->_host);
}

std::string	ServerParse::getName() const
{
	return (this->_name);
}

std::string	ServerParse::getRoot() const
{
	return (this->_root);
}

std::string	ServerParse::getAuto() const
{
	return (this->_autoindex);
}

std::string	ServerParse::getIndex() const
{
	return (this->_index);
}

std::string	ServerParse::getSize() const
{
	return (this->_client_max_body_size);
}

const std::map<int, std::string>&	ServerParse::getPage() const
{
	return (this->_error_page);
}

const std::vector<LocationParse*>		ServerParse::getLocation() const
{
	return (this->_location);
}
