CC		= sparc-gaisler-elf-gcc
# we use gcc for linking, as it appears to pass extra stuff to ld which we don't know about
LD		= sparc-gaisler-elf-gcc

CFLAGS		:= -mcpu=leon3 -mfix-gr712rc
CFLAGS		+= -mhard-float
CFLAGS		+= -O2
CFLAGS		+= -nostartfiles
CFLAGS		+= -std=gnu89
CFLAGS		+= -W -Wall -Wextra -Wshadow -Wuninitialized
CFLAGS		+= -pedantic
CFLAGS		+= -Wcast-qual -Wformat=2
CFLAGS		+= -fdiagnostics-show-option
CFLAGS		+= -Werror

CFLAGS          += -D_POSIX_CLOCK_SELECTION     #
CFLAGS          += -D_POSIX_MONOTONIC_CLOCK     # clock_gettime, _nanosleep()
CFLAGS          += -D_POSIX_TIMERS              #
CFLAGS          += -DDPU_TARGET
CFLAGS          += -D__sparc__
#CFLAGS		+= --coverage

LDFLAGS         := -nostartfiles -nostdlib -Wl,-T linker.lds
LDFLAGS		+= -Ttext=0x64000000
LDFLAGS		+= -Wl,--unresolved-symbols=ignore-in-object-files
#LDFLAGS	+= --coverage

SOURCEDIR	= src
BUILD_DIR 	= build

# myapp includes
APP_DIR	= src
APP_INC	= $(APP_DIR)

# CrIa Sources
APP_SRC	= $(shell find $(APP_DIR)/ -type f -name "*.c"| sort | sed /ext/d)
APP_OBJ	= $(APP_SRC:$(APP_DIR)/%.c=$(BUILD_DIR)/%.o)

INCLUDES 	= -I$(APP_INC) -I../libc/src
LIBS		= ../libc/build/libc.a

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

.SECONDEXPANSION:
$(APP_OBJ) : $$(patsubst $(BUILD_DIR)/%.o,$(APP_DIR)/%.c,$$@)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<

app:  $(BUILD_DIR) $(APP_OBJ)
	@echo "Building target."

	$(LD)  $(LDFLAGS) -o $(BUILD_DIR)/myapp $(APP_OBJ) $(LIBS)

        # we must keep the _start symbol so the OS can locate the entry point
	sparc-gaisler-elf-strip -s -K _start  $(BUILD_DIR)/myapp

	@echo "myapp is ready!"


.PHONY: all
all:	app
	@echo "Finished building target."

.PHONY: clean
clean:
	rm -f $(APP_OBJ) $(CRFW_OBJ) $(BUILD_DIR)/*.[ao] $(BUILD_DIR)/myapp
