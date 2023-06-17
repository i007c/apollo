
CC = gcc
CFLAGS = -std=c11 -O3 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
CFLAGS += -Wno-pointer-arith -Wno-newline-eof -Wno-unused-parameter
CFLAGS += -Wno-gnu-statement-expression -Wno-gnu-compound-literal-initializer 
CFLAGS += -Wno-gnu-zero-variadic-macro-arguments -Isrc/include/
LDFLAGS = -lm -ldl -lpthread -lGL -lGLU -lglut -lX11 -lpthread -lXrandr -lXi -ldl

SRC =  $(wildcard src/**/*.c) $(wildcard src/*.c)
SRC += $(wildcard src/**/**/*.c) $(wildcard src/**/**/**/*.c)
OBJ = $(SRC:.c=.o)
BIN = bin


all: clear dirs apollo

dirs:
	mkdir -p ./$(BIN)

run: all
	$(BIN)/apollo

apollo: $(OBJ)
	$(CC) -o $(BIN)/apollo $^ $(LDFLAGS)

%.o: %.c
	@$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf $(BIN) $(OBJ)

clear:
	printf "\E[H\E[3J"
	clear

.PHONY: clear run clean
.SILENT: clear run dirs apollo clean
