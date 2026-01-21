#include "LocationParse.hpp"

///////////CONSTRUCTOR / DESTRUCTOR///////////

LocationParse::LocationParse()
{
    this->_inLocation = false;
    this->_name = "";
    this->_autoindex = "";
    this->_root = "";
    this->_return[0] = "";
    this->_return[1] = "";
    this->_allowedMethods = std::vector<std::string>();
}

LocationParse::LocationParse(LocationParse const & copy)
{
	*this = copy;
}

LocationParse& LocationParse::operator=(LocationParse const & copy)
{
	if (this != &copy)
	{
        this->_inLocation = copy._inLocation;
        this->_name = copy._name;
        this->_autoindex = copy._autoindex;
        this->_root = copy._root;
        this->_return[0] = copy._return[0];
        this->_return[1] = copy._return[1];
        this->_allowedMethods = copy._allowedMethods;
	}
	return *this;
}

LocationParse::~LocationParse(){}


///////////////////LOCATION///////////////////

void LocationParse::parseLocation(std::ifstream& file, std::string line)
{
    this->_inLocation = true;
    if (line.find("location") != std::string::npos)
    {
        std::vector<std::string> tokens;
        tokens = splitTokens(line.c_str());
        std::string name = tokens[1];
        this->_name = name;
    } 
    while (getline(file, line))
    {
        if (line.empty())
            continue;
        if (line.find("}") != std::string::npos)
        {
            this->_inLocation = false;
            break;
        }  
        if (line.find("#") != std::string::npos)
			continue ;
        else if (line.find("allowed_methods") != std::string::npos)
        {
            parseAllowedMethods(line);
            continue ;
        }    
        else if (line.find("autoindex") != std::string::npos)
        {
            parseAuto(line);
            continue ;
        }    
        else if (line.find("root") != std::string::npos)
        {
            parseRoot(line);
            continue ;
        } 
        else if (line.find("return") != std::string::npos)
        {
            if (!_allowedMethods.empty() || !_autoindex.empty() || !_root.empty())
                throw Config::InvalidLocation();
            parseReturn(line);
            continue ;
        }
        else
        {
            std::cerr << line << std::endl;
            break ;
        }
    }
}

std::vector<std::string>	LocationParse::splitTokens(const char* str)
{
	std::vector<std::string> tokens;

    char* token = std::strtok((char *)str, " \n\t\r\v\f");
    while (token != NULL) {
        tokens.push_back(token);
        token = std::strtok(NULL, " \n\t\r\v\f");
    }
    return tokens;
}

/////////////////////NAME/////////////////////

void	LocationParse::parseName(std::string line)
{
	if (line.empty())
    {	
        std::cerr << CYAN << "[Location] Directive name: ";
		throw Config::NotEnoughArguments();
	}
    std::vector<std::string> token;
	token = splitTokens(line.c_str());
    std::string directive = token[1];
    this->_name = token[1];
}

////////////////////METHODS///////////////////

void LocationParse::parseAllowedMethods(std::string line)
{
    if (line.empty())
    {	
        std::cerr << CYAN << "[Location] Allowed_Methods: ";
		throw Config::NotEnoughArguments();
	}
    if (line[line.length() - 1] == ';')
    {
        line.erase(line.length() - 1);
    }
    std::vector<std::string> tokens;
	tokens = splitTokens(line.c_str());
    for (size_t i = 1; i < tokens.size(); ++i)
    {
        std::string method = tokens[i];
        if (method == "GET" || method == "POST" || method == "DELETE")
            this->_allowedMethods.push_back(method);
        else
        {		
            std::cerr << CYAN << "[Location] Allowed_Methods: ";
            throw Config::InvalidArgument();
        }
    }
}

/////////////////////ROOT/////////////////////

void	LocationParse::parseRoot(std::string line)
{
	if (line.empty())
	{
		std::cerr << CYAN << "[Location] Root: ";
		throw Config::NotEnoughArguments();
	}
    if (line[line.length() - 1] == ';')
    {
        line.erase(line.length() - 1);
    }
    std::vector<std::string>    token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {
		std::cerr << CYAN << "[Location] Root: ";
        throw Config::NotEnoughArguments();
    }
	std::string	root = token[1];
	
	struct stat sb;
	if (stat(root.c_str(), &sb) == 0)
        this->_root = root;
	else
    {
		std::cerr << CYAN << "[Location] Root: ";
        throw Config::InvalidArgument();
    }
}

/////////////////////AUTO/////////////////////

void	LocationParse::parseAuto(std::string line)
{
	if (line.empty())
	{	
		std::cerr << CYAN << "[Location] Autoindex: ";
		throw Config::NotEnoughArguments();
	}
    if (line[line.length() - 1] == ';')
    {
        line.erase(line.length() - 1);
    }
    std::vector<std::string>    token;
	token = splitTokens(line.c_str());
    if (token.size() != 2)
    {
		std::cerr << CYAN << "[Location] Autoindex: ";
        throw Config::NotEnoughArguments();
    }
	std::string	autoindex = token[1];
	if (autoindex == "on" || autoindex == "off")
        this->_autoindex = autoindex;
	else
    {
		std::cerr << CYAN << "[Location] Autoindex: ";
        throw Config::InvalidArgument();
    }
}

////////////////////RETURN////////////////////

void	LocationParse::parseReturn(std::string line)
{
    if (line.empty())
    {	
		std::cerr << CYAN << "[Location] Return: ";

		throw Config::NotEnoughArguments();
	}
    if (line[line.length() - 1] == ';')
    {
        line.erase(line.length() - 1);
    }
	std::vector<std::string> token;
	token = splitTokens(line.c_str());
	if (token.size() != 3)
    {
        std::cerr << CYAN << "[Location] Return: ";
        throw Config::InvalidArgument();
    }
    std::string status = token[1];
    std::string path = token[2];
    if (status == "301" || status == "302")
    {
        this->_return[0] = status;
        this->_return[1] = path;
    }
    else
    {
		std::cerr << CYAN << "[Location] Return: ";
        throw Config::InvalidArgument();
    }
}

////////////////////GETTER////////////////////

std::string	LocationParse::getName() const
{
	return (this->_name);
}

std::vector<std::string> LocationParse::getMethod() const
{
    return (this->_allowedMethods);
}

std::string	LocationParse::getRoot() const
{
	return (this->_root);
}

std::string	LocationParse::getAuto() const
{
	return (this->_autoindex);
}

std::string		LocationParse::getReturnStatus() const
{
    return (this->_return[0]);
}

std::string		LocationParse::getReturnPath() const
{
    return (this->_return[1]);
}
