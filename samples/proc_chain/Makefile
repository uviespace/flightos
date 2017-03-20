# kbuild trick to avoid linker error. Can be omitted if a module is built.
obj- := dummy.o

hostprogs-$(CONFIG_SAMPLE_PROC_CHAIN) := proc_chain_demo

# I guess I'm too stupid to figure out the proper way to do this
# (but maybe there is none)

ifdef CROSS_COMPILE
HOSTCC := $(CROSS_COMPILE)gcc
HOSTLD := $(CROSS_COMPILE)ld
endif


HOSTCFLAGS_proc_chain_demo.o += -I$(objtree)/include
proc_chain_demo-objs := proc_chain_demo.o

ifndef CROSS_COMPILE
EXTRAPFLAG = -m32
else
EXTRAPFLAG =
endif

HOSTCFLAGS_proc_chain_demo.o +=  $(EXTRAFLAG)
HOSTLOADLIBES_proc_chain_demo += $(EXTRAFLAG) $(objtree)/lib/lib.a

always := $(hostprogs-y)
