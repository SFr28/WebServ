#ifndef WEBSERV_HPP
# define WEBSERV_HPP

//Librairies
# include <iostream>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/epoll.h>
# include <netinet/in.h>
# include <unistd.h>
# include <map>
# include <vector>
# include <string>
# include <cstring>
# include <cstdlib>
# include <sstream>
# include <fstream>
# include <cerrno>
# include <cstdio>
# include <arpa/inet.h>
# include <cstddef>
# include <fcntl.h>
# include <dirent.h>
# include <ctime>
# include <algorithm>
# include <filesystem>
# include "serverSocket.hpp"
# include "ServerParse.hpp"
# include "LocationParse.hpp"
# include "Macro_CodeErrors.hpp"


//Color
# define RESET			"\033[0m"
# define BLACK			"\033[30m"
# define RED			"\033[31m"
# define GREEN			"\033[32m"
# define YELLOW			"\033[33m"
# define BLUE			"\033[34m"
# define MAGENTA		"\033[35m"
# define CYAN			"\033[36m"
# define WHITE			"\033[37m"
# define BOLD			"\033[1m"
# define B_BLACK		"\033[1m\033[30m"
# define B_RED			"\033[1m\033[31m"
# define B_GREEN		"\033[1m\033[32m"
# define B_YELLOW		"\033[1m\033[33m"
# define B_BLUE			"\033[1m\033[34m"
# define B_MAGENTA		"\033[1m\033[35m"
# define B_CYAN			"\033[1m\033[36m"
# define B_WHITE		"\033[1m\033[37m"

//MACRO
# define MAX_URI_LENGTH	4096

//UTILS
std::string &		toLower_string(std::string & str);
std::string			toStr(size_t nb);
std::string			generateDate();
bool				isDirectory(std::string const & path);
bool				pathExists(std::string const & path);
std::string			identifyMIME(std::string & path);
std::string			identifyReasonPhrase(int status);
bool				isAutoIndexAllowed(const LocationParse* location, const ServerParse& server);
void                ft_ctrl_c(int signum);

#endif
