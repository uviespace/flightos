# kbuild trick to avoid linker error. Can be omitted if a module is built.
obj- := dummy.o

hostprogs-$(CONFIG_SAMPLE_SYSCTL) := sysctl_demo

# I guess I'm too stupid to figure out the proper way to do this
# (but maybe there is none)

ifdef CROSS_COMPILE
HOSTCC := $(CROSS_COMPILE)gcc
HOSTLD := $(CROSS_COMPILE)ld
endif


HOSTCFLAGS_sysctl_demo.o += -I$(objtree)/include
sysctl_demo-objs := sysctl_demo.o

ifndef CROSS_COMPILE
EXTRAPFLAG = -m32
else
EXTRAPFLAG =
endif

HOSTCFLAGS_sysctl_demo.o +=  $(EXTRAFLAG)
HOSTLOADLIBES_sysctl_demo += $(EXTRAFLAG) $(objtree)/lib/lib.a

always := $(hostprogs-y)
