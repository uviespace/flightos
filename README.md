# Vienna Flight OS

This is a development snapshot of the Vienna Flight OS intended for future use
in software contributions by the
[space instrumentation working group](https://space.univie.ac.at)
at the Department of Astrophysics of the University of Vienna.

The current focus is on LEON3/LEON4 SPARC processors, but ports to different
architectures (such as RISC-V) may be added in the future if required for our
purposes.

## A Brief History

This is a development which has its roots in an evaluation study
of the MPPB (Massively Parallel Processor Breadboarding) prototype hardware
and was later expanded to target the SSDP (Scalable Sensor Data Processor),
which was intended to become a next-generation space-rated
system-on-chip that included Xentium DSPs in a Network-On-Chip which were
to be controlled by a general-purpose LEON3 host CPU.

While the SSDP has been abandoned, the extensions needed to operate the NoC and
the DSPs were never more than modular components (and are in fact still part
of the source tree) of the operating system so its development is continued.

Please note that this was originally referred to as *LeanOS* as a working title,
but since the name was not very descriptive of its intended purpose, it was
recently changed to *Vienna Flight OS*. References in pre-rendered versions of
the PDF documentation are thus still referring to LeanOS.

# Documentation

See Documentation/ subdirectory.


## Build instructions

(extremely brief, will expand later)

### Prerequisites

**NOTE** only relase 2.1.2 of bcc2 is confirmed to work properly, v2.0.7 produces
bad instructions for at least one instance (arch/sparc/kernel/bootmem.c),
v2.2.0 appears to have major problems, which are yet to be identified in detail
but appear to be related to the stack offset calculations or register usage, resulting
in infinite frames due to recursively entering the original code segment
when a window_overflow trap is triggered.


- bcc or bcc2 (preferred) tool chain
- GR712 or GR740 develpment board
- grmon

### Building

The build system is based on a modified variant of *kbuild* as used with
the Linux kernel, so you can configure the build with *make menuconfig*.

To run the copy performance/ threaded multicore demonstrator, copy
gr712_demo-config or gr740_demo-config to .config, then run *make*.

You may need to change the cross-compiler prefix for your particular tool chain
(default is *sparc-gaisler-elf-*). This can be achieved by editing the .config
file or by running *make menuconfig* and in the *General Setup* submenu.

If you run into issues with cpu-specific compiler flags, you may want to
have a look at *arch/sparc/Makefile* and change the options accordingly.


# References

For an overview of the development history (in order), see:

<https://indico.esa.int/event/60/contributions/2757/attachments/2284/2639/DSP_Day_2014_-_NGAPP_final_presentation__RUAG-A.pdf>

<https://indico.esa.int/event/102/contributions/39/attachments/152/177/5_2_P_A_Lightweight_Operating_System_for_the_SSDP_20160613.pdf>

<https://indico.esa.int/event/182/contributions/1513/attachments/1430/1655/1220_-_Luntzer.pdf>

<http://microelectronics.esa.int/userday-gr740/16_10-Armin_Luntzer_gr740userday2019.pdf>

