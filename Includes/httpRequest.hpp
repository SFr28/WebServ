#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "webserv.hpp"
# include "httpResponse.hpp"
# include "Config.hpp"

class httpResponse;

class httpRequest
{
	private:
		std::vector<ServerParse>			_serversList;
		ServerParse*						_server;
		std::vector<std::string>			_data;
		std::string							_printRequest;
		std::string							_method;
		std::string							_target;
		std::string							_version;
		std::map<std::string, std::string>	_headers;
		std::vector<char>					_body;
		int									_statusCode;
		int									_contentLength;
		std::string							_contentType;
		bool								_isParsed;

		bool	parseRawData(std::vector<char> &data);
		bool	readFirstLine();
		bool	checkMethod(std::string & method);
		int		readMethod(std::string &firstLine);
		int		readTarget(std::string &firstLine);
		int		readVersion(std::string &firstLine);

		bool	readHeaderFields();
		bool	checkForContentLength();
		bool	checkForContentType();
		bool	checkForHost();
		bool	isHostValid(std::string &host);

		bool	checkBody();

	public:
		httpRequest(std::vector<char> &data, std::vector<ServerParse> servers);
		httpRequest(int statusCode, std::vector<ServerParse> servers);
		~httpRequest();

		std::vector<std::string> const &			getData() const;
		std::string const &							getprintRequest() const;
		std::string const &							getMethod() const;
		std::string const & 						getTarget() const;
		std::string const & 						getVersion() const;
		int	const &									getStatusCode() const;
		std::map<std::string, std::string> const &	getHeader() const;
		std::vector<char> const &					getBody() const;
		int const &									getContentLength() const;
		std::string const &							getContentType() const;
		bool										getIsParsed() const;
		const ServerParse&							getServer() const;
};

#endif