#ifndef _KERNEL_ELF_H_
#define _KERNEL_ELF_H_


#include <stdint.h>
#include <stddef.h>

/* some ELF constants&stuff ripped from include/uapi/linux/elf.h */


/* 32-bit base types */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

/* 64-bit base types */
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef  int16_t Elf64_SHalf;
typedef uint64_t Elf64_Off;
typedef  int32_t Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef  int64_t Elf64_Sxword;


__extension__
typedef struct elf32_dyn {
	Elf32_Sword d_tag;		/* dynamic array tag */
	union{
		Elf32_Sword	d_val;
		Elf32_Addr	d_ptr;	/* program virtual address */
	};
} Elf32_Dyn;

__extension__
typedef struct elf64_dyn {
	Elf64_Sxword d_tag;		/* entry tag value */
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	};
} Elf64_Dyn;

/* The following are used with relocations */
#define ELF32_R_SYM(x)		((x) >> 8)
#define ELF32_R_TYPE(x)		((x) & 0xff)

#define ELF64_R_SYM(x)		((x) >> 32)
#define ELF64_R_TYPE(x)		((x) & 0xffffffff)

/* Dynamic Array Tags */
#define DT_NULL		0		/* end of _DYNAMIC array */
#define DT_NEEDED	1		/* str tbl offset of needed librart */
#define DT_PLTRELSZ	2               /* size of relocation entries in PLT */
#define DT_PLTGOT	3               /* address PLT/GOT */
#define DT_HASH		4               /* address of symbol hash table */
#define DT_STRTAB	5               /* address of string table */
#define DT_SYMTAB	6               /* address of symbol table */
#define DT_RELA		7               /* address of relocation table */
#define DT_RELASZ	8               /* size of relocation table */
#define DT_RELAENT	9               /* size of relocation entry */
#define DT_STRSZ	10              /* size of string table */
#define DT_SYMENT	11              /* size of symbol table entry */
#define DT_INIT		12              /* address of initialization func. */
#define DT_FINI		13              /* address of termination function */
#define DT_SONAME	14              /* string tbl offset of shared obj */
#define DT_RPATH 	15              /* str tbl offset of lib search path */
#define DT_SYMBOLIC	16              /* start symbol search in shared obj */
#define DT_REL	        17              /* address of reloc tbl with addends */
#define DT_RELSZ	18              /* size of DT_REL relocation table */
#define DT_RELENT	19              /* size of DT_REL relocation entry */
#define DT_PLTREL	20              /* PLT referenced relocation entry */
#define DT_DEBUG	21              /* used for debugging */
#define DT_TEXTREL	22              /* allow reloc mod to unwritable seg */
#define DT_JMPREL	23              /* addr of PLT's relocation entries */
#define DT_ENCODING	32
#define OLD_DT_LOOS	0x60000000
#define DT_LOOS		0x6000000d
#define DT_HIOS		0x6ffff000
#define DT_VALRNGLO	0x6ffffd00
#define DT_VALRNGHI	0x6ffffdff
#define DT_ADDRRNGLO	0x6ffffe00
#define DT_ADDRRNGHI	0x6ffffeff
#define DT_VERSYM	0x6ffffff0
#define DT_RELACOUNT	0x6ffffff9
#define DT_RELCOUNT	0x6ffffffa
#define DT_FLAGS_1	0x6ffffffb
#define DT_VERDEF	0x6ffffffc
#define	DT_VERDEFNUM	0x6ffffffd
#define DT_VERNEED	0x6ffffffe
#define	DT_VERNEEDNUM	0x6fffffff
#define OLD_DT_HIOS     0x6fffffff
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff


/* Dynamic Flags - DT_FLAGS_1 .dynamic entry */
#define DF_1_NOW       0x00000001
#define DF_1_GLOBAL    0x00000002
#define DF_1_GROUP     0x00000004
#define DF_1_NODELETE  0x00000008
#define DF_1_LOADFLTR  0x00000010
#define DF_1_INITFIRST 0x00000020
#define DF_1_NOOPEN    0x00000040
#define DF_1_ORIGIN    0x00000080
#define DF_1_DIRECT    0x00000100
#define DF_1_TRANS     0x00000200
#define DF_1_INTERPOSE 0x00000400
#define DF_1_NODEFLIB  0x00000800
#define DF_1_NODUMP    0x00001000
#define DF_1_CONLFAT   0x00002000

/* ld.so: number of low tags that are used saved internally (0 .. DT_NUM-1) */
#define DT_NUM        (DT_JMPREL+1)

/* This info is needed when parsing the symbol table */
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_COMMON  5
#define STT_TLS     6

#define ELF_ST_BIND(x)		((x) >> 4)
#define ELF_ST_TYPE(x)		(((unsigned int) x) & 0xf)
#define ELF32_ST_BIND(x)	ELF_ST_BIND(x)
#define ELF32_ST_TYPE(x)	ELF_ST_TYPE(x)
#define ELF64_ST_BIND(x)	ELF_ST_BIND(x)
#define ELF64_ST_TYPE(x)	ELF_ST_TYPE(x)



typedef struct elf32_rel {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
} Elf32_Rel;

typedef struct elf64_rel {
	Elf64_Addr r_offset;	/* Location at which to apply the action */
	Elf64_Xword r_info;	/* index and type of relocation */
} Elf64_Rel;

typedef struct elf32_rela{
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
} Elf32_Rela;

typedef struct elf64_rela {
	Elf64_Addr r_offset;	/* Location at which to apply the action */
	Elf64_Xword r_info;	/* index and type of relocation */
	Elf64_Sxword r_addend;	/* Constant addend used to compute value */
} Elf64_Rela;


/* relocs as needed */
#define R_AMD64_NONE     0
#define R_AMD64_RELATIVE 8




typedef struct elf32_sym{
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	Elf32_Half	st_shndx;
} Elf32_Sym;

typedef struct elf64_sym {
	Elf64_Word	st_name;	/* Symbol name, index in string tbl */
	unsigned char	st_info;	/* Type and binding attributes */
	unsigned char	st_other;	/* No defined meaning, 0 */
	Elf64_Half	st_shndx;	/* Associated section index */
	Elf64_Addr	st_value;	/* Value of the symbol */
	Elf64_Xword	st_size;	/* Associated symbol size */
} Elf64_Sym;














/*
 * Note Definitions
 */
typedef struct {
    Elf32_Word namesz;
    Elf32_Word descsz;
    Elf32_Word type;
} Elf32_Note;

typedef struct {
    Elf64_Half namesz;
    Elf64_Half descsz;
    Elf64_Half type;
} Elf64_Note;





/**
 * e_ident[] identification indexes
 */

#define EI_MAG0		0	/* file ID */
#define EI_MAG1		1	/* file ID */
#define EI_MAG2		2	/* file ID */
#define EI_MAG3		3	/* file ID */
#define EI_CLASS	4	/* file class */
#define EI_DATA		5	/* data encoding */
#define EI_VERSION	6	/* ELF header version */
#define EI_OSABI	7	/* OS/ABI ID */
#define EI_ABIVERSION	8	/* ABI version */
#define EI_PAD		9	/* start of pad bytes */
#define EI_NIDENT	16	/* size of e_ident[] */

/* e_ident[] magic numbers */
#define ELFMAG0		0x7f		/* e_ident[EI_MAG0] */
#define ELFMAG1		'E'		/* e_ident[EI_MAG1] */
#define ELFMAG2		'L'		/* e_ident[EI_MAG2] */
#define ELFMAG3		'F'		/* e_ident[EI_MAG3] */
#define ELFMAG		"\177ELF"	/* magic */
#define SELFMAG		4		/* size of magic */

/* e_ident[] file classes */
#define ELFCLASSNONE	0	/* invalid */
#define ELFCLASS32	1	/* 32-bit objs */
#define ELFCLASS64	2	/* 64-bit objs */
#define ELFCLASSNUM	3	/* number of classes */

/* e_ident[] data encoding */
#define ELFDATANONE	0	/* invalid */
#define ELFDATA2LSB	1	/* Little-Endian */
#define ELFDATA2MSB	2	/* Big-Endian */
#define ELFDATANUM	3	/* number of data encode defines */

/* e_ident[] Operating System/ABI */
#define ELFOSABI_SYSV		0	/* UNIX System V ABI */
#define ELFOSABI_HPUX		1	/* HP-UX operating system */
#define ELFOSABI_NETBSD		2	/* NetBSD */
#define ELFOSABI_LINUX		3	/* GNU/Linux */
#define ELFOSABI_HURD		4	/* GNU/Hurd */
#define ELFOSABI_86OPEN		5	/* 86Open common IA32 ABI */
#define ELFOSABI_SOLARIS	6	/* Solaris */
#define ELFOSABI_MONTEREY	7	/* Monterey */
#define ELFOSABI_IRIX		8	/* IRIX */
#define ELFOSABI_FREEBSD	9	/* FreeBSD */
#define ELFOSABI_TRU64		10	/* TRU64 UNIX */
#define ELFOSABI_MODESTO	11	/* Novell Modesto */
#define ELFOSABI_OPENBSD	12	/* OpenBSD */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */


/* e_version */
#define EV_NONE		0
#define EV_CURRENT	1
#define EV_NUM		2	/* number of versions */

/* e_type */
#define ET_NONE		0
#define ET_REL		1	/* relocatable */
#define ET_EXEC		2	/* executable */
#define ET_DYN		3	/* shared object*/
#define ET_CORE		4	/* core file */
#define ET_NUM		5	/* number of types */
#define ET_LOPROC	0xff00	/* reserved range for processor */
#define ET_HIPROC	0xffff	/*  specific e_type */

/* e_machine */
#define EM_NONE		0
#define EM_M32		1	/* AT&T WE 32100 */
#define EM_SPARC	2	/* SPARC */
#define EM_386		3	/* Intel 80386 */
#define EM_68K		4	/* Motorola 68000 */
#define EM_88K		5	/* Motorola 88000 */
#define EM_486		6	/* Intel 80486 - unused? */
#define EM_860		7	/* Intel 80860 */
#define EM_MIPS		8	/* MIPS R3000 Big-Endian only */
#define EM_MIPS_RS4_BE	10	/* MIPS R4000 Big-Endian */
#define EM_SPARC64	11	/* SPARC v9 64-bit unofficial */
#define EM_PARISC	15	/* HPPA */
#define EM_SPARC32PLUS	18	/* Enhanced instruction set SPARC */
#define EM_PPC		20	/* PowerPC */
#define EM_ARM		40	/* ARM AArch32 */
#define EM_ALPHA	41	/* DEC ALPHA */
#define EM_SH		42	/* Hitachi/Renesas Super-H */
#define EM_SPARCV9	43	/* SPARC version 9 */
#define EM_IA_64	50	/* Intel IA-64 Processor */
#define EM_AMD64	62	/* AMD64 architecture */
#define EM_VAX		75	/* DEC VAX */
#define EM_AARCH64	183	/* ARM AArch64 */


/* ELF32 Header */
typedef struct elf32_hdr{
	unsigned char	e_ident[EI_NIDENT];	/* ELF identifier */
	Elf32_Half	e_type;			/* object file type */
	Elf32_Half	e_machine;		/* machine type */
	Elf32_Word	e_version;		/* object file version */
	Elf32_Addr	e_entry;		/* entry point */
	Elf32_Off	e_phoff;		/* program header offset */
	Elf32_Off	e_shoff;		/* section header offset */
	Elf32_Word	e_flags;		/* processor-specific flags */
	Elf32_Half	e_ehsize;		/* ELF header size */
	Elf32_Half	e_phentsize;		/* program header entry size */
	Elf32_Half	e_phnum;		/* number of program headers */
	Elf32_Half	e_shentsize;		/* section header entry size */
	Elf32_Half	e_shnum;		/* number of section headers */
	Elf32_Half	e_shstrndx;		/* string table index */
} Elf32_Ehdr;

/* ELF64 Header */
typedef struct elf64_hdr{
	unsigned char	e_ident[EI_NIDENT];	/* ELF identifier */
	Elf64_Half	e_type;			/* object file type */
	Elf64_Half	e_machine;		/* machine type */
	Elf64_Word	e_version;		/* object file version */
	Elf64_Addr	e_entry;		/* entry point */
	Elf64_Off	e_phoff;		/* program header offset */
	Elf64_Off	e_shoff;		/* section header offset */
	Elf64_Word	e_flags;		/* processor-specific flags */
	Elf64_Half	e_ehsize;		/* ELF header size */
	Elf64_Half	e_phentsize;		/* program header entry size */
	Elf64_Half	e_phnum;		/* number of program headers */
	Elf64_Half	e_shentsize;		/* section header entry size */
	Elf64_Half	e_shnum;		/* number of section headers */
	Elf64_Half	e_shstrndx;		/* string table index */
} Elf64_Ehdr;


/* ELF32 Program Header */
typedef struct elf32_phdr{
	Elf32_Word	p_type;			/* entry type */
	Elf32_Off	p_flags;		/* flags */
	Elf32_Addr	p_offset;		/* file offset */
	Elf32_Addr	p_vaddr;		/* virtual address */
	Elf32_Word	p_paddr;		/* physical address */
	Elf32_Word	p_filesz;		/* size in file */
	Elf32_Word	p_memsz;		/* size in memory */
	Elf32_Word	p_align;		/* alignment, file & memory */
} Elf32_Phdr;

/* ELF64 Program Header */
typedef struct elf64_phdr {
	Elf64_Word	p_type;			/* entry type */
	Elf64_Word	p_flags;		/* flags */
	Elf64_Off	p_offset;		/* file offset */
	Elf64_Addr	p_vaddr;		/* virtual address */
	Elf64_Addr	p_paddr;		/* physical address */
	Elf64_Xword	p_filesz;		/* size in file */
	Elf64_Xword	p_memsz;		/* size in memory */
	Elf64_Xword	p_align;		/* alignment, file & memory */
} Elf64_Phdr;


/* Program entry types */
#define PT_NULL		0		/* unused */
#define PT_LOAD		1		/* loadable segment */
#define PT_DYNAMIC	2		/* dynamic linking section */
#define PT_INTERP	3		/* the RTLD */
#define PT_NOTE		4		/* auxiliary information */
#define PT_SHLIB	5		/* reserved - purpose undefined */
#define PT_PHDR		6		/* program header */
#define PT_TLS		7		/* thread local storage */
#define PT_LOOS		0x60000000	/* reserved range for OS */
#define PT_HIOS		0x6fffffff	/*  specific segment types */
#define PT_LOPROC	0x70000000	/* reserved range for processor */
#define PT_HIPROC	0x7fffffff	/*  specific segment types */


/* ELF32 Section Header */
typedef struct elf32_shdr {
	Elf32_Word	sh_name;		/* section name index */
	Elf32_Word	sh_type;		/* section type */
	Elf32_Word	sh_flags;		/* section flags */
	Elf32_Addr	sh_addr;		/* (virtual) address */
	Elf32_Off	sh_offset;		/* file offset */
	Elf32_Word	sh_size;		/* section size */
	Elf32_Word	sh_link;		/* link to other section header */
	Elf32_Word	sh_info;		/* misc info */
	Elf32_Word	sh_addralign;		/* memory address alignment */
	Elf32_Word	sh_entsize;		/* section entry size */
} Elf32_Shdr;

/* ELF64 Section Header */
typedef struct elf64_shdr {
	Elf64_Word	sh_name;		/* section name */
	Elf64_Word	sh_type;		/* section type */
	Elf64_Xword	sh_flags;		/* section flags */
	Elf64_Addr	sh_addr;		/* (virtual) address */
	Elf64_Off	sh_offset;		/* file offset */
	Elf64_Xword	sh_size;		/* section size */
	Elf64_Word	sh_link;		/* link to other section header */
	Elf64_Word	sh_info;		/* misc info */
	Elf64_Xword	sh_addralign;		/* memory address alignment */
	Elf64_Xword	sh_entsize;		/* section entry size */
} Elf64_Shdr;


/* Special Section Indexes */
#define SHN_UNDEF     0             /* undefined */
#define SHN_LORESERVE 0xff00        /* lower bounds of reserved indexes */
#define SHN_LOPROC    0xff00        /* reserved range for processor */
#define SHN_HIPROC    0xff1f        /*   specific section indexes */
#define SHN_ABS       0xfff1        /* absolute value */
#define SHN_COMMON    0xfff2        /* common symbol */
#define SHN_HIRESERVE 0xffff        /* upper bounds of reserved indexes */

/* sh_type */
#define SHT_NULL	0		/* inactive */
#define SHT_PROGBITS	1		/* program defined information */
#define SHT_SYMTAB	2		/* symbol table section */
#define SHT_STRTAB	3		/* string table section */
#define SHT_RELA	4		/* relocation section with addends*/
#define SHT_HASH	5		/* symbol hash table section */
#define SHT_DYNAMIC	6		/* dynamic section */
#define SHT_NOTE	7		/* note section */
#define SHT_NOBITS	8		/* no space section */
#define SHT_REL		9		/* relation section without addends */
#define SHT_SHLIB	10		/* reserved */
#define SHT_DYNSYM	11		/* dynamic symbol table section */
#define SHT_NUM		12		/* number of section types */
#define SHT_LOPROC	0x70000000	/* reserved range for processor */
#define SHT_HIPROC	0x7fffffff	/*  specific section header types */
#define SHT_LOUSER	0x80000000	/* reserved range for application */
#define SHT_HIUSER	0xffffffff	/*  specific indexes */

/* Section names */
#define ELF_BSS         ".bss"        /* uninitialized data */
#define ELF_DATA        ".data"       /* initialized data */
#define ELF_DEBUG       ".debug"      /* debug */
#define ELF_DYNAMIC     ".dynamic"    /* dynamic linking information */
#define ELF_DYNSTR      ".dynstr"     /* dynamic string table */
#define ELF_DYNSYM      ".dynsym"     /* dynamic symbol table */
#define ELF_FINI        ".fini"       /* termination code */
#define ELF_GOT         ".got"        /* global offset table */
#define ELF_HASH        ".hash"       /* symbol hash table */
#define ELF_INIT        ".init"       /* initialization code */
#define ELF_REL_DATA    ".rel.data"   /* relocation data */
#define ELF_REL_FINI    ".rel.fini"   /* relocation termination code */
#define ELF_REL_INIT    ".rel.init"   /* relocation initialization code */
#define ELF_REL_DYN     ".rel.dyn"    /* relocation dynamic link info */
#define ELF_REL_RODATA  ".rel.rodata" /* relocation read-only data */
#define ELF_REL_TEXT    ".rel.text"   /* relocation code */
#define ELF_RODATA      ".rodata"     /* read-only data */
#define ELF_SHSTRTAB    ".shstrtab"   /* section header string table */
#define ELF_STRTAB      ".strtab"     /* string table */
#define ELF_SYMTAB      ".symtab"     /* symbol table */
#define ELF_TEXT        ".text"       /* code */


/* Section Attribute Flags - sh_flags */
#define SHF_WRITE        0x1           /* Writable */
#define SHF_ALLOC        0x2           /* occupies memory */
#define SHF_EXECINSTR    0x4           /* executable */
#define SHF_TLS          0x400         /* thread local storage */
#define SHF_MASKPROC     0xf0000000    /* reserved bits for processor
                                        *  specific section attributes */


/* Sparc ELF relocation types */
#define	R_SPARC_NONE		0
#define	R_SPARC_8		1
#define	R_SPARC_16		2
#define	R_SPARC_32		3
#define	R_SPARC_DISP8		4
#define	R_SPARC_DISP16		5
#define	R_SPARC_DISP32		6
#define	R_SPARC_WDISP30		7
#define	R_SPARC_WDISP22		8
#define	R_SPARC_HI22		9
#define	R_SPARC_22		10
#define	R_SPARC_13		11
#define	R_SPARC_LO10		12
#define	R_SPARC_GOT10		13
#define	R_SPARC_GOT13		14
#define	R_SPARC_GOT22		15
#define	R_SPARC_PC10		16
#define	R_SPARC_PC22		17
#define	R_SPARC_WPLT30		18
#define	R_SPARC_COPY		19
#define	R_SPARC_GLOB_DAT	20
#define	R_SPARC_JMP_SLOT	21
#define	R_SPARC_RELATIVE	22
#define	R_SPARC_UA32		23
#define R_SPARC_PLT32		24
#define R_SPARC_HIPLT22		25
#define R_SPARC_LOPLT10		26
#define R_SPARC_PCPLT32		27
#define R_SPARC_PCPLT22		28
#define R_SPARC_PCPLT10		29
#define R_SPARC_10		30
#define R_SPARC_11		31
#define R_SPARC_64		32
#define R_SPARC_OLO10		33
#define R_SPARC_WDISP16		40
#define R_SPARC_WDISP19		41
#define R_SPARC_7		43
#define R_SPARC_5		44
#define R_SPARC_6		45




/* end of ELF stuff */



/* begin configurable section */

typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Phdr Elf_Phdr;
typedef Elf32_Dyn  Elf_Dyn;
typedef Elf32_Rel  Elf_Rel;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Off  Elf_Off;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Sym  Elf_Sym;

#define ELF_R_SYM(x)    ELF32_R_SYM(x)
#define ELF_R_TYPE(x)   ELF32_R_TYPE(x)



/**
 * NOTE: no free configuration, limited to 32bit ELF, SPARCv8, big endian,
 * because I am lazy :)
 */
#if 1
#define ELF_ENDIAN	ELFDATA2MSB
#define EM_MACHINE	EM_SPARC
#else
#define ELF_ENDIAN	ELFDATA2LSB
#define EM_MACHINE	EM_AMD64
#endif


#define elf_check_arch(x) ((x)->e_machine == EM_MACHINE)
#define elf_check_endian(x) ((x)->e_ident[EI_DATA] == ELF_ENDIAN)


/* implemented in arch code */
int elf_header_check(Elf_Ehdr *ehdr);


#endif /* _KERNEL_ELF_H_ */
