CC		= sparc-gaisler-elf-gcc
# we use gcc for linking, as it appears to pass extra stuff to ld which we don't know about
LD		= sparc-gaisler-elf-gcc

#CFLAGS		= -O2 -W -Wall -Wextra -Werror -std=gnu89 -pedantic
#CFLAGS		=  -r -mv8 -mhard-float -mfix-gr712rc -O2 -std=gnu89 -W -Wall
CFLAGS		=  -mcpu=leon3 -mfix-gr712rc -mhard-float -O3 -std=gnu89  -nostartfiles
#CFLAGS		=  -mcpu=leon3 -mfix-gr712rc -mhard-float -O3 -std=gnu89 -W -Wall \
		   -Wextra -Werror  -Wshadow -Wuninitialized \
		   -fdiagnostics-show-option -Wcast-qual -Wformat=2 \
		   -Wno-unused -Wno-type-limits \
		   -Wno-format -nostartfiles #-pedantic #-nostartfiles -nostdlib

CFLAGS		+= -DDPU_TARGET -DDPU_TARGET__GLUE
CFLAGS		+= -D_POSIX_TIMERS #clock_gettime()
LDFLAGS		= -nostartfiles -Ttext=0x60000000  -nostdlib
#LDFLAGS		=  -nostartfiles -Wl,--unresolved-symbols=ignore-in-object-files  -Wl,-emain -Ttext=0x60000000 

#LDFLAGS		= -r  -nostartfiles -nostdlib  -Map=dpu.map
#LDFLAGS		= -Xlinker -Ttext=0x40480000 -Xlinker -Tdata=0x40500000 -Xlinker -Tbss=0x40520000

SOURCEDIR	= src
BUILD_DIR 	= build


# myapp includes
APP_DIR	= .
APP_INC	= $(APP_DIR)

# RMAP, OS/glue code
EXT_INC		= $(APP_DIR)/ext


# CrIa Sources
APP_SRC	= $(shell find $(APP_DIR)/ -type f -name "*.c"| sort | sed /ext/d)
APP_OBJ	= $(APP_SRC:$(APP_DIR)/%.c=$(BUILD_DIR)/%.o)

INCLUDES 	= -I$(EXT_INC) -I$(APP_INC)
LIBS		=


$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)


.SECONDEXPANSION:
$(APP_OBJ) : $$(patsubst $(BUILD_DIR)/%.o,$(APP_DIR)/%.c,$$@)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<

app:  $(BUILD_DIR) $(APP_OBJ)
	@echo "Building target."

	$(LD) $(LIBS) $(LDFLAGS) -o $(BUILD_DIR)/myapp $(APP_OBJ)

	sparc-gaisler-elf-strip --strip-debug  $(BUILD_DIR)/myapp

	@echo "myapp is ready!"


.PHONY: all
all:	app
	@echo "Finished building target."

.PHONY: clean
clean:
	rm -f $(APP_OBJ) $(CRFW_OBJ) $(BUILD_DIR)/*.[ao] $(BUILD_DIR)/myapp
