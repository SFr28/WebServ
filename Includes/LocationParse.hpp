#ifndef LOCATIONPARSE_HPP
# define LOCATIONPARSE_HPP

# include "webserv.hpp"

class LocationParse
{
	private:
		bool    					_inLocation;
		std::string					_name;
		std::vector<std::string>	_allowedMethods;
		std::string					_autoindex;
		std::string					_root;
		std::string					_return[2];

		void						parseName(std::string line);
		void    					parseAllowedMethods(std::string line);
		void    					parseAuto(std::string line);
		void    					parseRoot(std::string line);
		void						parseReturn(std::string line);
		std::vector<std::string>	splitTokens(const char* str);

	public:
		LocationParse();
		LocationParse(LocationParse const & copy);
		LocationParse&	operator=(LocationParse const & copy);
		~LocationParse();

		void						parseLocation(std::ifstream& file, std::string line);

		std::string					getName() const;
		std::vector<std::string>	getMethod() const;
		std::string					getRoot() const;
		std::string					getAuto() const;
		std::string					getReturnStatus() const;
		std::string					getReturnPath() const;
};

#endif