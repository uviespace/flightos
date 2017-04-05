# kbuild trick to avoid linker error. Can be omitted if a module is built.
obj- := dummy.o

hostprogs-$(CONFIG_SAMPLE_NOC_DMA) := noc_dma_demo

# I guess I'm too stupid to figure out the proper way to do this
# (but maybe there is none)

ifdef CROSS_COMPILE
HOSTCC := $(CROSS_COMPILE)gcc
HOSTLD := $(CROSS_COMPILE)ld
endif


HOSTCFLAGS_noc_dma_demo.o += -I$(objtree)/include -include include/generated/autoconf.h
HOSTCFLAGS_noc_dma_demo.o += -include include/generated/autoconf.h
noc_dma_demo-objs := noc_dma_demo.o

ifndef CROSS_COMPILE
EXTRAPFLAG = -m32
else
EXTRAPFLAG =
endif

HOSTCFLAGS_noc_dma_demo.o +=  $(EXTRAFLAG)
HOSTLOADLIBES_noc_dma_demo += $(EXTRAFLAG) $(objtree)/lib/lib.a
HOSTLOADLIBES_noc_dma_demo += $(objtree)/kernel/noc_dma.o
HOSTLOADLIBES_noc_dma_demo += $(objtree)/kernel/printk.o
HOSTLOADLIBES_noc_dma_demo += $(objtree)/kernel/irq.o

always := $(hostprogs-y)
