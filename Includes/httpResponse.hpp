#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

#include "webserv.hpp"
#include "httpRequest.hpp"
#include "LocationParse.hpp"

class httpRequest;

class Config;
class ServerParse;
class LocationParse;

class httpResponse
{
	private:
		const ServerParse*					_server;
		httpRequest &						_request;
		std::string							_version;
		int									_statusCode;
		std::string							_reasonPhrase;
		std::map<std::string, std::string>	_headerFields;
		std::vector<char>					_body;
		std::vector<char>					_response;
		bool								_CGI;
		size_t								_lenResponse;
		std::string							_path;
		bool								_delete;
		std::map<std::string,std::string>	_postHeaders;
		std::map<std::string,std::string>	_cookies;
		LocationParse*						_location;
		bool								_isCGI;

		bool		checkMethod(std::string const & method);
		std::string	checkLocation(std::string const & uri);
		bool		checkPath(std::string const & uri);
		bool		checkCGI();
		bool		handleDirectory(std::string & path);
		bool		generateAutoindex(std::string const & path);
		bool		foundBody();
		bool		parsePost();
		bool		parseFormData(std::map<std::string, std::string> & headers);
		bool		makePostFile(const char* data, size_t size, const char* headerStart, size_t headerSize);
		void		makeCookie();

		void		handleMethod();
		bool		handleGET();
		bool		handlePOST();
		bool		handleDELETE();
				
		public:
		httpResponse(httpRequest &request, std::map<std::string, std::string> & cookies);
		~httpResponse();
		
		void	handleError();
		void 	handleCGIError();

		void	finalizeCGIResponse(std::vector<char>& buffer);
		void	generateResponse();

		std::vector<char> const &				getResponse(void) const;
		size_t const &							getLenResponse(void) const;
		std::map<std::string, std::string> &	getCookies();
		bool const &							getIsCGI() const;
		std::string &							getPath();
		ServerParse const &						getServer() const;
		int										getStatusCode() const;
		void									setStatusCode(int statusCode);

};

#endif
