SOURCES = src/*.c
COMP_FLAGS = -g `gimptool-2.0 --cflags`
LINK_FLAGS = `gimptool-2.0 --libs`
OUTPUT_DIR = $(HOME)/.gimp-2.6/plug-ins
NAME = dct-carver

install:
	cc $(COMP_FLAGS) $(SOURCES) -o $(OUTPUT_DIR)/$(NAME) $(LINK_FLAGS)

clean:
	rm -rf *~ ./src/*~ ./src/*.orig

codestyle:
	astyle -TaSfUp src/*

uninstall:
	rm $(OUTPUT_DIR)/$(NAME)

