CCPREFIX	= sparc-gaisler-elf-
CC		= $(CCPREFIX)gcc
# we use gcc for linking, as it appears to pass extra stuff to ld which we don't know about
LD		= $(CCPREFIX)gcc

AR              = $(CCPREFIX)ar
ARFLAGS         = crs

CFLAGS		:= -mcpu=leon3 -mfix-gr712rc
CFLAGS		+= -O2
CFLAGS		+= -mhard-float
# make sure to disable incorrect recursion/inlining optimisation
CFLAGS		+= --param max-inline-recursive-depth=0 -fno-inline-functions -fno-optimize-sibling-calls --param min-inline-recursive-probability=100
CFLAGS		+= -std=gnu89
CFLAGS		+= -W -Wall -Wextra -Wshadow -Wuninitialized
CFLAGS		+= -pedantic
#CFLAGS		+= -Wcast-qual -Wformat=2
CFLAGS		+= -fdiagnostics-show-option
CFLAGS		+= -Werror
CFLAGS		+= -DDPU_TARGET
CFLAGS		+= -DUSE_OPTIMISATIONS

LDFLAGS		=

INCLUDES	= -Isrc

SOURCE_DIR	= src
BUILD_DIR 	= build

LIBC_SRC	= $(shell find $(SOURCE_DIR)/ -type f -name "*.[c]")
LIBC_OBJ	= $(LIBC_SRC:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)
LIBC_ASM	= $(shell find $(SOURCE_DIR)/ -type f -name "*.[S]")
LIBC_ASMOBJ	= $(LIBC_ASM:$(SOURCE_DIR)/%.S=$(BUILD_DIR)/%.o)



.PHONY: all
all:	libc

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

.SECONDEXPANSION:
$(LIBC_OBJ) : $$(patsubst $(BUILD_DIR)/%.o,$(SOURCE_DIR)/%.c,$$@)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<

.SECONDEXPANSION:
$(LIBC_ASMOBJ) : $$(patsubst $(BUILD_DIR)/%.o,$(SOURCE_DIR)/%.S,$$@)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<


libc:  $(LIBC_OBJ) $(LIBC_ASMOBJ) $(BUILD_DIR)
	$(AR) $(ARFLAGS) $(BUILD_DIR)/libc.a $(LIBC_OBJ) $(LIBC_ASMOBJ)


.PHONY: clean
clean:
	rm -f $(LIBC_OBJ) $(LIBC_ASMOBJ) $(BUILD_DIR)/*.[ao]

