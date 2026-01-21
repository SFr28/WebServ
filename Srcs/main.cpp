#include "Config.hpp"

int g_signal = 0;

int main(int ac, char **av)
{
	Config		config;
	std::string configFile;

	if (ac > 2)
		return (-1);
	if (ac == 2)
		configFile = av[1];
	else
		configFile = "default.conf";
	try
	{
		config.parseConfig(configFile);
		
		serverSocket server(config);
		signal(SIGINT, &ft_ctrl_c);
		server.initialize();
	}
	catch(const std::exception& e)
	{
		std::cerr << RED << e.what() << RESET <<'\n';
	}
	return (0);
}
