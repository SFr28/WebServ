#ifndef SERVERPARSE_HPP
# define SERVERPARSE_HPP

# include "LocationParse.hpp"

class LocationParse;

class ServerParse
{
	private:
		bool							_inServer;
		std::string						_listen;
		std::string						_host;
		std::string						_name;
		std::string						_root;
		std::string						_autoindex;
		std::string						_index;
		std::string						_client_max_body_size;
		std::map<int, std::string>		_error_page;
		std::vector<LocationParse*>		_location;

		void							parsePort(std::string line);			// PORT
		void							parseHost(std::string line);			// HOST
		void							parseName(std::string line);			// NAME
		void							parseRoot(std::string line);			// ROOT
		void							parseIndex(std::string line);			// INDEX
		void							parseAuto(std::string line);			// AUTOINDEX
		void							parseSize(std::string line);			// CLIENT_MAX_BODY_SIZE
		void							parsePage(std::string line);			// ERROR_PAGE

		std::vector<std::string>		splitTokens(const char* str);

	public:
		ServerParse();
		ServerParse(ServerParse const & copy);
		ServerParse&	operator=(ServerParse const & copy);
		~ServerParse();

		void								parseServer(std::ifstream& file, std::string line);

		std::string							getListen() const;
		std::string							getHost() const;
		std::string							getName() const;
		std::string							getRoot() const;
		std::string							getAuto() const;
		std::string							getIndex() const;
		std::string							getSize() const;
		const std::map<int, std::string>&	getPage() const;
		const std::vector<LocationParse*>	getLocation() const;
};

#endif