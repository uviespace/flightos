# we just hijack the host build system because it's the easiest way...
#
# Xentium kernels are built as executables, with library functions
# explicitly listed as object dependencies. The executables are later collected
# in scripts/link-leanos.sh and added to the embedded AR image.
#
# The address of the .text section for the first xentium kernel is determined
# by the confiuration in dsp/xentium/sysroot/lib/default.ld, all subsequent
# programs will start after the _end symbol of the previous
#
# To achieve this, list dsp kernel targets in hostprogs-y and configure targets
# so that the next target depends on the finalised previous target and take
# the value of the _end symbol as the new .text start
#
# This isn't a pretty solution, but that's what you get for not supporting
# relocations. It works for now and it is at least slightly more comfortable
# than configuring all text base addresses by hand
#


obj- := dummy.o


HOSTCC := xentium-clang
HOSTLD := xentium-ld

HOSTCFLAGS = -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -Idsp/xentium
	     
HOSTLDFLAGS += -Tdsp/xentium/sysroot/lib/default.ld
HOSTLDFLAGS += --sysroot=dsp/xentium/sysroot


hostprogs-y := testkernel.xen
hostprogs-y += otherkernel.xen



testkernelkernel.xen :
HOSTLOADLIBES_testkernel.xen := 
testkernel.xen-objs := testkernel.o xen_printf.o


otherkernel.xen : testkernel.xen
HOSTLOADLIBES_otherkernel.xen := -Ttext $$(readelf -s dsp/xentium/testkernel.xen|grep -w _end |awk '{print $$2}') 
otherkernel.xen-objs := testkernel.o xen_printf.o




always := $(hostprogs-y)
