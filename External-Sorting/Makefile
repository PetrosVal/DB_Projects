OBJS	= sort.o main.o
OUT		= sort
CC		= gcc
FLAGS	= -c -g -Wall -w
LIB 	= BF_64.a

# test HT
sort.o: sort.c
	$(CC) $(FLAGS) sort.c
main.o: main.c
	$(CC) $(FLAGS) main.c
all: $(OBJS)
	$(CC) $(OBJS) $(LIB) -o $(OUT)

# clean up
clean:
	rm -f $(OBJS) $(OUT)