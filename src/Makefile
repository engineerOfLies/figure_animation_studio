##############################################################################
#
# The Linux-GCC Makefile
#
##############################################################################


#
# Compiler stuff -- adjust to your system.
#

# Linux
PROJECT = fas
CC      = gcc

LIB_LIST = ../gameframework2d/libs/libgf2d.a ../gameframework2d/gfc/libs/libgfc.a ../gameframework2d/gfc/simple_json/libs/libsj.a ../gameframework2d/gfc/simple_logger/libs/libsl.a 
OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

INC_PATHS = ../include ../gameframework2d/include ../gameframework2d/gfc/include ../gameframework2d/gfc/simple_logger/include ../gameframework2d/gfc/simple_json/include
INC_PARAMS =$(foreach d, $(INC_PATHS), -I$d)

OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

SDL_CFLAGS = `sdl2-config --cflags` $(INC_PARAMS)
SDL_LDFLAGS = `sdl2-config --libs` -lSDL2_image -lpng -ljpeg -lz -lSDL2_ttf -lSDL2_mixer -lm
LFLAGS = -g  -o ../$(PROJECT) -lm
CFLAGS = -g  -fPIC -Wall -pedantic -std=gnu99 -fgnu89-inline -Wno-unknown-pragmas -Wno-variadic-macros -Wformat-truncation=0 

DOXYGEN = doxygen

#
# Targets
#

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) $(LIB_LIST) $(SDL_LDFLAGS) 

docs:
	$(DOXYGEN) doxygen.cfg

sources:
	echo (patsubst %.c,%.o,$(wildcard *.c)) > makefile.sources


makefile.dep: depend

depend:
	@touch makefile.dep
	@-rm makefile.dep
	@echo Creating dependencies.
	@for i in *.c; do $(CC) $(INC) -MM $(INC_PARAMS) $$i; done > makefile.dep
	@echo Done.

clean:
	rm *.o

count:
	wc -l *.c $(foreach d, $(INC_PATHS), $d/*.h) makefile

#
# Dependencies.
#

include makefile.dep

#
# The default rule.
#

.c.o:
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $<


