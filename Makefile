NAME = webserv

CPP = c++
CPP_FLAG = -Wall -Werror -Wextra -I ./Includes -std=c++98 -MMD -MP -g

FILE =	Srcs/main.cpp					

RESPONSE =	Srcs/response/httpResponse.cpp	\
			Srcs/response/post.cpp			\
			Srcs/response/get.cpp			\
			Srcs/response/cookie.cpp		

REQUEST =	Srcs/sockets/serverSocket.cpp	\
			Srcs/request/httpRequest.cpp	\
			Srcs/handle_cgi/HandleCGI.cpp

CONFIG =	Srcs/config/Config.cpp			\
			Srcs/config/LocationParse.cpp	\
			Srcs/config/ServerParse.cpp

UTILS = Srcs/utils.cpp
		
FILE +=	$(RESPONSE) $(REQUEST) $(CONFIG) $(UTILS)
		
O_FILE = $(FILE:%.cpp=$(O_DIR)%.o)

O_DIR = Objects/

DEPS = $(O_FILE:%.o=%.d)

UPLOAD = ./www/uploads/
all: $(NAME)
	
$(NAME): $(O_FILE)
	$(CPP) $(CPP_FLAG) $(O_FILE) -o $(NAME)

$(O_DIR)%.o: %.cpp
	@mkdir -p $(dir $@) $(UPLOAD)
	$(CPP) $(CPP_FLAG) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(O_FILE)

fclean: clean
	rm -rf $(O_DIR) $(UPLOAD) $(NAME)

re: fclean all

.PHONY: all clean fclean re

