CC = gcc
CFLAGS = -Wall
OBJS = main.o
BIN = main

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)
	clear
	./$(BIN)
	
%.o: %.c
	$(CC) $(CFLAGS)	-c $< -o $@
		
clean:
	rm -f $(BIN) $(OBJS)
	clear