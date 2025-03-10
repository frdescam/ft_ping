.SUFFIXES:
.SUFFIXES: .c .o

SHELL		= /bin/sh

NAME		= ft_ping

SRCS		= ./src/main.c ./src/ping.c

OBJS		= ${SRCS:.c=.o}

CC			= cc
RM			= rm -f

CFLAGS		= -Wall -Wextra -Werror -g3 -std=gnu99

all:	${NAME}

.c.o:
		${CC} ${CFLAGS} -c $< -o $@

$(NAME): ${OBJS}
		${CC} ${OBJS} -o ${NAME} 
clean:
		${RM} ${OBJS}


fclean:	clean
		${RM} ${NAME}

re:		fclean
	make all

.PHONY : all clean fclean re
