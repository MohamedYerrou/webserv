SRCS = main.cpp srcs/run_server.cpp srcs/Client.cpp srcs/Server.cpp srcs/Location.cpp

OBJ =  $(SRCS:.cpp=.o)

CC = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = webserv

all : $(NAME)

%.o : %.cpp
	$(CC) $(CFLAGS) -Iincludes -c $< -o $@

$(NAME) : $(OBJ)
	$(CC) $(OBJ) -o $(NAME)

clean :
	rm -f $(OBJ)

fclean : clean
	rm -f $(NAME)

re : fclean all

.PHONY:  all clean fclean re