CC = gcc

CFLAGS = -Wall -Wextra -std=c99

SRC = vm.c

EXEC = vm

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC) -lm

clean:
	rm -f $(EXEC)
