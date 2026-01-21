#include "Config.hpp"

///////////CONSTRUCTOR / DESTRUCTOR///////////

Config::Config()
{
	this->_nbServ = 0;
}

Config::Config(Config const & copy)
{
    *this = copy;
}

Config&  Config::operator=(Config const & copy)
{
    if (this != &copy)
    {
		this->_nbServ = copy._nbServ;
		this->_server = copy._server;
    }
    return *this;
}

Config::~Config(){}

////////////////////CONFIG////////////////////

void Config::parseConfig(std::string filename)
{
	std::string line;

	std::ifstream file(filename.c_str());
	if (!file)
	{
		std::cerr << "Can't open file" << std::endl;
		return ;
	}
	while (getline(file, line))
	{
		if (line.find("#") != std::string::npos)
			continue ;
		if (line.find("server {") != std::string::npos)
		{
			ServerParse serv;
			serv.parseServer(file, line);
			this->_server.push_back(serv);
		}
		
		else if (line.find("}") != std::string::npos)
		{
			this->_nbServ++;
			break ;
		}
		
    }
}

////////////////////GETTER////////////////////

int	Config::getNbServer() const
{
	return (this->_nbServ);
}

const std::vector<ServerParse>&	Config::getServer() const
{
	return (this->_server);
}

//////////////////EXCEPTIONS//////////////////

const char* Config::NotEnoughArguments::what() const throw()
{
	return ("Wrong amount of argument(s).");
}

const char* Config::InvalidArgument::what() const throw()
{
	return ("Invalid argument(s).");
}
		
const char* Config::InvalidLocation::what() const throw()
{
	return ("Invalid location.");
}
