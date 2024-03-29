
CC = gcc
CFLAGS  = -std=c11 -Os -g -pedantic -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Isrc/include/  -Ilib/

LDFLAGS = -lGL -lGLU -lm -lglfw
# -ldl -lXrandr -lXi -lX11 -lpthread -lglfw -lGLEW
# -lglut 

SOURCES = $(shell find src -type f -name "*.c" -not -path "src/.ccls-cache/*")
HEADERS = $(shell find src -type f -name "*.h" -not -path "src/.ccls-cache/*")
OBJECTS = $(addprefix build/, $(SOURCES:.c=.o))
EXEC = bin/apollo


$(EXEC): clear $(OBJECTS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)


build/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $< -o $@
	@echo $<


run: clear $(EXEC)
	$(EXEC)


clean:
	rm -rf $(EXEC) $(OBJECTS)


clear:
	printf "\E[H\E[3J"
	clear

.PHONY: clear run clean
.SILENT: clear run clean $(EXEC)

