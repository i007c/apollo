
CC = gcc
CFLAGS  = -std=c11 -O0 -g -pedantic -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Isrc/include/  -Ilib/ -D_GNU_SOURCE

LDFLAGS = -lm -lglfw -lvulkan
# -ldl -lXrandr -lXi -lX11 -lpthread -lglfw -lGLEW
# -lglut 

SOURCES = $(shell find src -type f -name "*.c" -not -path "src/.ccls-cache/*" -not -name "app.c")
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


run: clear $(EXEC) shader/spv/compute.spv
	$(EXEC)


shader/spv/compute.spv:
	@glslc -fshader-stage=compute shader/compute.glsl -o $@
	@# glslc -fshader-stage=frag shader/fragment.glsl  -o shader/spv/fragment.spv
	@# glslc -fshader-stage=vert shader/vertex.glsl    -o shader/spv/vertex.spv

clean:
	rm -rf $(EXEC) $(OBJECTS)


clear:
	printf "\E[H\E[3J"
	clear

.PHONY: clear run clean shader/spv/compute.spv
.SILENT: clear run clean $(EXEC)

