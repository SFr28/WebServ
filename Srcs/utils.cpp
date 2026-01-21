#include "webserv.hpp"

extern int	g_signal;

//Return the string in lowercase
std::string &	toLower_string(std::string & str)
{
	for (int i = 0; str[i]; i++)
		str[i] = std::tolower(str[i]);
	
	return (str);
}

//Convert a size_t in string
std::string	toStr(size_t nb)
{
	std::ostringstream out;
    out << nb;
    return out.str();
}

//Return a string with the actual date in http format(GMT)
std::string	generateDate()
{
	std::string	date;
	time_t		now	= time(NULL);
	struct tm*	dateStruct = gmtime(&now);
	char		buffer[128];

	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", dateStruct);
	date = buffer;
	
	return date;
}

//Check if the given path is a directory
bool	isDirectory(std::string const & path)
{
	struct stat data_path;
	
	if (stat(path.c_str(), &data_path) == -1)
		return false;

	return (S_ISDIR(data_path.st_mode));
}

//Check if the given path exists
bool	pathExists(std::string const & path)
{
	struct stat data_file;
	
	if (stat(path.c_str(), &data_file) == 0)
		return true;

	return false;
}

//Return the Content Type depending of the extension used
//If no extension is found or the extension is unknowned, return application/octet-stream
std::string	identifyMIME(std::string & path)
{
	static std::map<std::string, std::string> mimeTypes;

    if (mimeTypes.empty()) {
        mimeTypes["html"] = "text/html";
        mimeTypes["htm"] = "text/html";
        mimeTypes["css"] = "text/css";
        mimeTypes["js"] = "application/javascript";
        mimeTypes["json"] = "application/json";
        mimeTypes["png"] = "image/png";
        mimeTypes["jpg"] = "image/jpeg";
        mimeTypes["jpeg"] = "image/jpeg";
        mimeTypes["gif"] = "image/gif";
        mimeTypes["svg"] = "image/svg+xml";
        mimeTypes["txt"] = "text/plain";
        mimeTypes["pdf"] = "application/pdf";
        mimeTypes["zip"] = "application/zip";
        mimeTypes["mp3"] = "audio/mpeg";
        mimeTypes["mp4"] = "video/mp4";
        mimeTypes["webm"] = "video/webm";
        mimeTypes["ico"] = "image/x-icon";
    }
	
	size_t		posExt = path.find_last_of('.');
	if (posExt == std::string::npos)
		return "application/octet-stream";
	
	std::string	extension = path.substr(posExt + 1);
	extension = toLower_string(extension);

	std::map<std::string, std::string>::const_iterator	it = mimeTypes.find(extension);
	if (it != mimeTypes.end())
		return it->second;

	return "application/octet-stream";
}

//Return the Reason Phrase based on the statusCode
std::string	identifyReasonPhrase(int status)
{
	std::map<int, std::string>	dicoStatus;

	dicoStatus[CONTINUE] = "Continue";
	dicoStatus[SWITCH] = "Switch";
	dicoStatus[OK] = "Ok";
	dicoStatus[ACCEPTED] = "Accepted";
	dicoStatus[NAI] = "Non-Authoritative Information";
	dicoStatus[NO_CONTENT] = "No Content";
	dicoStatus[RESET_CONTENT] = "Reset Content";
	dicoStatus[PARTIAL_CONTENT] = "Partial Content";
	dicoStatus[MULTIPLE_CHOICE] = "Multiple Choice";
	dicoStatus[MOVED_PERMANENTLY] = "Moved Permanently";
	dicoStatus[FOUND] = "Found";
	dicoStatus[SEE_OTHER] = "See Other";
	dicoStatus[NOT_MODIFIED] = "Not Modified";
	dicoStatus[USE_PROXY] = "Use Proxy";
	dicoStatus[TEMP_REDIRECT] = "Temporary Redirect";
	dicoStatus[PERM_REDIRECT] = "Permanent Redirect";
	dicoStatus[BAD_REQUEST] = "Bad Request";
	dicoStatus[UNAUTHORIZED] = "Unauthorized";
	dicoStatus[PAYMENT_REQUIRED] = "Payment Required";
	dicoStatus[FORBIDDEN] = "Forbidden";
	dicoStatus[NOT_FOUND] = "Not Found";
	dicoStatus[METHOD_NOT_ALLOWED] = "Method Not Allowed";
	dicoStatus[NOT_ACCEPTABLE] = "Not Acceptable";
	dicoStatus[PAR] = "Proxy Authentification Required";
	dicoStatus[REQUEST_TIMEOUT] = "Request Timeout";
	dicoStatus[CONFLICT] = "Conflict";
	dicoStatus[GONE] = "Gone";
	dicoStatus[LENGTH_REQUIRED] = "Length Required";
	dicoStatus[PRECONDITION_FAILED] = "Precondition Failed";
	dicoStatus[CONTENT_TOO_LARGE] = "Content Too Large";
	dicoStatus[URI_TOO_LONG] = "URI Too Long";
	dicoStatus[UNSUPPORTED_MEDIA_TYPE] = "Unsupported Media Type";
	dicoStatus[RANGE_NOT_SATISFIABLE] = "Requested range unsatisfiable";
	dicoStatus[EXPECTATION_FAILED] = "Expectation Failed";
	dicoStatus[MISDIRECTED_REQUEST] = "Misdirected Request";
	dicoStatus[UNPROCESSABLE_CONTENT] = "Unprocessable Content";
	dicoStatus[UPGRADE_REQUIRED] = "Upgrade Required";
	dicoStatus[INTERNAL_SERVER_ERROR] = "Internal Server Error";
	dicoStatus[NOT_IMPLEMENTED] = "Not Implemented";
	dicoStatus[BAD_GATEWAY] = "Bad Gateway";
	dicoStatus[SERVICE_UNAVAILABLE] = "Service Unavailable";
	dicoStatus[GATEWAY_TIMEOUT] = "Gateway Timeout";
	dicoStatus[HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported";

	return dicoStatus[status];
}

//Check if AutoIndex is allowed for a request
bool	isAutoIndexAllowed(const LocationParse* location, const ServerParse& server)
{
	if (location != NULL && location->getAuto().empty() == false)
	{
		if (location->getAuto() == "on")
			return true;
		else
			return false;
	}
	if (server.getAuto() == "on")
		return true;
	return false;
}

//Signal handler for Ctrl-C
void ft_ctrl_c(int signum)
{
	if (signum == SIGINT)
		g_signal = 1;
}

