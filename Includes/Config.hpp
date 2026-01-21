#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "webserv.hpp"
# include "ServerParse.hpp"
# include "LocationParse.hpp"

class Config
{
	private:
		int							_nbServ;
		std::vector<ServerParse>	_server;

	public:
		Config();
		Config(Config const & copy);
		Config&	operator=(Config const & copy);
		~Config();

		void	parseConfig(std::string filename);

		int		getNbServer() const;
		const std::vector<ServerParse>&	getServer() const;

		class	NotEnoughArguments : public std::exception
		{
			public:
				const char* what() const throw();
		};

		class	InvalidArgument : public std::exception
		{
			public:
				const char* what() const throw();
		};

		class	InvalidLocation : public std::exception
		{
			public:
				const char* what() const throw();
		};
};

#endif
