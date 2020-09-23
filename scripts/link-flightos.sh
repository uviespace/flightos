#!/bin/sh
#
# link flightos
#
# flightos is linked from the objects selected by $(KBUILD_FLIGHTOS_INIT) and
# $(KBUILD_FLIGHTOS_MAIN). Most are built-in.o files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
# Ordering when linking is important, and $(KBUILD_FLIGHTOS_INIT) must be first.
#
# flightos
#   ^
#   |
#   +-< $(KBUILD_FLIGHTOS_INIT)
#   |   +--< init/version.o + more
#   |
#   +--< $(KBUILD_FLIGHTOS_MAIN)
#   |    +--< drivers/built-in.o mm/built-in.o + more
#   |
#   +-< ${kallsymso} (see description in KALLSYMS section)
#
# flightos version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update flightos.
# Therefore this step is delayed until just before final link of flightos.
#
# System.map is generated to document addresses of all kernel symbols

# Error out on error
set -e

# Nice output in kbuild format
# Will be supressed by "make -s"
info()
{
	if [ "${quiet}" != "silent_" ]; then
		printf "  %-7s %s\n" ${1} ${2}
	fi
}

# Thin archive build here makes a final archive with
# symbol table and indexes from flightos objects, which can be
# used as input to linker.
#
# Traditional incremental style of link does not require this step
#
# built-in.o output file
#
archive_builtin()
{
	if [ -n "${CONFIG_THIN_ARCHIVES}" ]; then
		info AR built-in.o
		rm -f built-in.o;
		${AR} rcsT${KBUILD_ARFLAGS} built-in.o			\
					${KBUILD_FLIGHTOS_INIT}		\
					${KBUILD_FLIGHTOS_MAIN}
	fi
}

# Link of flightos.o used for section mismatch analysis
# ${1} output file
modpost_link()
{
	local objects

	if [ -n "${CONFIG_THIN_ARCHIVES}" ]; then
		objects="--whole-archive built-in.o"
	else
		objects="${KBUILD_FLIGHTOS_INIT}				\
			--start-group					\
			${KBUILD_FLIGHTOS_MAIN}				\
			--end-group"
	fi
	${LD} ${LDFLAGS} -r -o ${1} ${objects}
}

# Link of flightos
# ${1} - optional extra .o files
# ${2} - output file
# ${3} - extra flags
flightos_link()
{
	local lds="${objtree}/${KBUILD_LDS}"


	# if we link against the (BCC) libc, we'll just
	# call $CC instead of LD
	if [ -n "${CONFIG_TARGET_COMPILER_BOOT_CODE}" ]; then
		${CC} ${LDFLAGS} ${LDFLAGS_flightos} ${3} -o ${2}		\
			${KBUILD_FLIGHTOS_INIT} ${KBUILD_FLIGHTOS_MAIN} ${1}
	fi

	if [ -n "${CONFIG_ARCH_CUSTOM_BOOT_CODE}" ]; then
		${LD} ${LDFLAGS} ${LDFLAGS_flightos} ${3} -o ${2}		\
			-T ${lds} ${KBUILD_FLIGHTOS_INIT} ${KBUILD_FLIGHTOS_MAIN} \
			${1}
	fi

}


# Create ${2} .o file with all symbols from the ${1} object file
kallsyms()
{
	info KSYM ${2}
	local kallsymopt;


	if [ -n "${CONFIG_HAVE_UNDERSCORE_SYMBOL_PREFIX}" ]; then
		kallsymopt="${kallsymopt} --symbol-prefix=_"
	fi

	if [ -n "${CONFIG_KALLSYMS_ALL}" ]; then
		kallsymopt="${kallsymopt} --all-symbols"
	fi

	if [ -n "${CONFIG_KALLSYMS_ABSOLUTE_PERCPU}" ]; then
		kallsymopt="${kallsymopt} --absolute-percpu"
	fi

	if [ -n "${CONFIG_KALLSYMS_BASE_RELATIVE}" ]; then
		kallsymopt="${kallsymopt} --base-relative"
	fi

	local aflags="${KBUILD_AFLAGS} ${KBUILD_AFLAGS_KERNEL}               \
		      ${NOSTDINC_FLAGS} ${LINUXINCLUDE} ${KBUILD_CPPFLAGS}"

	local afile="`basename ${2} .o`.c"

	${NM} -n ${1} | scripts/gen_ksym.awk > ${afile}
	#${NM} -n ${1} | scripts/kallsyms ${kallsymopt} > ${afile}
	${CC} ${aflags} -c -o ${2} ${afile}
}

# Create map file with all symbols from ${1}
# See mksymap for additional details
mksysmap()
{
	${CONFIG_SHELL} "${srctree}/scripts/mksysmap" ${1} ${2}
}

sortextable()
{
	${objtree}/scripts/sortextable ${1}
}

# Delete output files in case of error
cleanup()
{
	rm -f .old_version
	rm -f .tmp_System.map
	rm -f .tmp_kallsyms*
	rm -f .tmp_version
	rm -f .tmp_flightos*
	rm -f built-in.o
	rm -f System.map
	rm -f flightos
	rm -f flightos.o
}

on_exit()
{
	if [ $? -ne 0 ]; then
		cleanup
	fi
}
trap on_exit EXIT

on_signals()
{
	exit 1
}
trap on_signals HUP INT QUIT TERM

#
#
# Use "make V=1" to debug this script
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

if [ "$1" = "clean" ]; then
	cleanup
	exit 0
fi

# We need access to CONFIG_ symbols
case "${KCONFIG_CONFIG}" in
*/*)
	. "${KCONFIG_CONFIG}"
	;;
*)
	# Force using a file from the current directory
	. "./${KCONFIG_CONFIG}"
esac

archive_builtin

#link flightos.o
info LD flightos.o
modpost_link flightos.o

# modpost flightos.o to check for section mismatches
${MAKE} -f "${srctree}/scripts/Makefile.modpost" flightos.o

# Update version
info GEN .version
if [ ! -r .version ]; then
	rm -f .version;
	echo 1 >.version;
else
	mv .version .old_version;
	expr 0$(cat .old_version) + 1 >.version;
fi;

# final build of init/
${MAKE} -f "${srctree}/scripts/Makefile.build" obj=init GCC_PLUGINS_CFLAGS="${GCC_PLUGINS_CFLAGS}"

kallsymso=""
kallsyms_flightos=""
if [ -n "${CONFIG_KALLSYMS}" ]; then

	# kallsyms support
	# Generate section listing all symbols and add it into flightos
	# It's a three step process:
	# 1)  Link .tmp_flightos1 so it has all symbols and sections,
	#     but __kallsyms is empty.
	#     Running kallsyms on that gives us .tmp_kallsyms1.o with
	#     the right size
	# 2)  Link .tmp_flightos2 so it now has a __kallsyms section of
	#     the right size, but due to the added section, some
	#     addresses have shifted.
	#     From here, we generate a correct .tmp_kallsyms2.o
	# 2a) We may use an extra pass as this has been necessary to
	#     woraround some alignment related bugs.
	#     KALLSYMS_EXTRA_PASS=1 is used to trigger this.
	# 3)  The correct ${kallsymso} is linked into the final flightos.
	#
	# a)  Verify that the System.map from flightos matches the map from
	#     ${kallsymso}.

	kallsymso=.tmp_kallsyms2.o
	kallsyms_flightos=.tmp_flightos2

	# step 1
	flightos_link "" .tmp_flightos1
	kallsyms .tmp_flightos1 .tmp_kallsyms1.o

	# step 2
	flightos_link .tmp_kallsyms1.o .tmp_flightos2
	kallsyms .tmp_flightos2 .tmp_kallsyms2.o

	# step 2a
	if [ -n "${KALLSYMS_EXTRA_PASS}" ]; then
		kallsymso=.tmp_kallsyms3.o
		kallsyms_flightos=.tmp_flightos3

		flightos_link .tmp_kallsyms2.o .tmp_flightos3

		kallsyms .tmp_flightos3 .tmp_kallsyms3.o
	fi
fi

info LD flightos
flightos_link "${kallsymso}" flightos

if [ -n "${CONFIG_BUILDTIME_EXTABLE_SORT}" ]; then
	info SORTEX flightos
	sortextable flightos
fi

info SYSMAP System.map
mksysmap flightos System.map

# step a (see comment above)
if [ -n "${CONFIG_KALLSYMS}" ]; then
	mksysmap ${kallsyms_flightos} .tmp_System.map

	if ! cmp -s System.map .tmp_System.map; then
		echo >&2 Inconsistent kallsyms data
		echo >&2 Try "make KALLSYMS_EXTRA_PASS=1" as a workaround
		exit 1
	fi
fi


# this is a 3rd pass option, we need modules.order beforehand
if [ "$1" = "embed" ]; then

	if [ ! -s ${srctree}/modules.order ] || [! -s ${CONFIG_EMBED_APPLICATION} ]; then
		echo >&2
		echo >&2 modules.order or application path empty or nonexistant, cannot embed image.
		echo >&2 Maybe you have no loadable modules configured or no executable path specified?
		echo >&2 Kernel image unchanged.
		echo >&2
		exit
	fi

	if [ -n "${CONFIG_TARGET_COMPILER_BOOT_CODE}" ]; then
		embedflags="-Wl,--format=binary -Wl,modules.image -Wl,--format=default"
	fi

	if [ -n "${CONFIG_ARCH_CUSTOM_BOOT_CODE}" ]; then
		embedflags="--format=binary modules.image --format=default"
	fi


	rm -f modules.image
	${AR} rcs ${srctree}/modules.image \
		$(tr '\n' ' ' < ${srctree}/modules.order) \
		$(find dsp/xentium -name *.xen) \
		"${CONFIG_EMBED_APPLICATION}"
		

	flightos_link "${kallsymso}" flightos "${embedflags}"
	exit
fi

# We made a new kernel - delete old version file
rm -f .old_version
