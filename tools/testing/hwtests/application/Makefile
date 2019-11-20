#CC               = sparc-elf-gcc
CC               = sparc-gaisler-elf-gcc
SOURCEDIR	 = ./
INCLUDEDIR       = ./
BUILDDIR         = ./
PATH            +=
CPPFLAGS        :=
#CFLAGS          := -r -mv8 -mhard-float -mfix-gr712rc -O2 -std=gnu89 -W -Wall
CFLAGS          := -r -mhard-float -mcpu=leon3 -std=gnu89 -W -Wall \
		   -Wextra -Werror -pedantic -Wshadow -Wuninitialized \
		   -fdiagnostics-show-option -Wcast-qual -Wformat=2 \
		   -nostartfiles
LDFLAGS         :=
SOURCES         := $(wildcard *.c) 
OBJECTS         := $(patsubst %.c, $(BUILDDIR)/%.o, $(subst $(SOURCEDIR)/,, $(SOURCES)))
TARGET          := executable_demo

DEBUG?=1
ifeq  "$(shell expr $(DEBUG) \> 1)" "1"
	    CFLAGS += -DDEBUGLEVEL=$(DEBUG)
else
	    CFLAGS += -DDEBUGLEVEL=1
endif


all: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $(TARGET)


