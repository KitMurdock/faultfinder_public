CC			= gcc
OBJ_DIR 	= obj
FINDER_DIR 	= finder
HUNTER_DIR 	= hunter
SHARED_DIR	= shared
FINDER_OBJ	= faultfinder.o
HUNTER_OBJ  = faulthunter.o
#CFLAGS		= -I. -I$(FINDER_DIR) -I$(SHARED_DIR) 
LIBS 		= -lunicorn -lpthread -lcapstone -ljson-c -lm

CFLAGS		= -g -I. -Wall
CFLAGS += $(shell pkg-config --cflags json-c)


LDFLAGS += $(shell pkg-config --libs json-c)

SOURCES		:=$(wildcard $(SHARED_DIR)/*.c)
OBJECTS		:=$(patsubst $(SHARED_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: faultfinder 

debug:	DEBUG= -DDEBUG
debug:	faultfinder

printinstructions: 	DEBUG=-DPRINTINSTRUCTIONS
printinstructions: 	faultfinder


$(OBJ_DIR)/%.o:	$(SHARED_DIR)/%.c
		$(info ************ SHARED $@ ************)
			$(CC) -O0 -c $< -o $@  $(CFLAGS) $(LIBS) $(DEBUG)


faultfinder: $(OBJECTS) $(FINDER_DIR)/faultfinder.o  /usr/local/lib/libunicorn.so.2
		$(info ************ FAULTFINDER ************)
			$(CC) -O0 -o $@  $^ $(CFLAGS) $(LIBS) $(DEBUG)


.PHONY: clean

clean:
	rm $(OBJ_DIR)/*.o finder/faultfinder.o faultfinder

