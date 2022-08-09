# gcc (4.x)
#ARCH_CFLAGS = -mcpu=v8 -mfix-gr712rc
# clang
#ARCH_CFLAGS = -mcpu=gr712rc


# gcc (7.2.0)
ifeq ($(CONFIG_LEON4),y)
KBUILD_CFLAGS  += -m32 -mcpu=leon3 -mhard-float --param max-inline-recursive-depth=0
endif

ifeq ($(CONFIG_LEON3),y)
ARCH_CFLAGS = -mcpu=leon3 -mfix-gr712rc -mhard-float --param max-inline-recursive-depth=0  -Wno-attribute-alias
endif



