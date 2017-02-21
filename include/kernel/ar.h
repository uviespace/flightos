#ifndef _KERNEL_AR_H_
#define _KERNEL_AR_H_

#include <kernel/types.h>

#define AR_MAG   "!<arch>\n"		/* AR MAGIC ID */
#define AR_FMAG  "`\n"			/* AR HDR END MAGIC */

#define AR_STR_TERM		'/'
#define GNU_AR_STR_TBL_MAG	"//"	/* marks a gnu archive string table */
#define GNU_AR_SYM_TBL_MAG	"/ "	/* marks a gnu archive symbol table */


struct ar_hdr {
	char ar_name[16];	/* ar file identifier */
	char ar_date[12];	/* file date, dec seconds since epoch */
	char ar_uid[6];		/* user  ID; dec  */
	char ar_gid[6];		/* group ID; dec  */
	char ar_mode[8];	/* file mode; oct */
	char ar_size[10];	/* file size; dec */
	char ar_fmag[2];	/* AR_FMAG (0x60, 0x0a) */
};


/**
 * The archive reference structure.
 */

struct archive {
	char  *ar_base;		/* the archive's memory location */
	unsigned long ar_size;	/* the size of the archive in bytes */
	char  *ar_str;		/* the archive's string table */

	unsigned int n_file;	/* the number of files in the archive */
	char **fname;		/* pointer array to the file name strings */
	char **p_file;		/* pointer array to the file(s) */
	unsigned long *filesz;	/* the size of each file */
	unsigned long *fnamesz;	/* the size of each file name */

	uint32_t n_sym;		/* the number of symbols in the index */
	char **sym;		/* pointer array to the symbol name strings */
	void **p_sym;		/* pointer array to the file a symbol is in */

};


void ar_list_files(struct archive *a);
void ar_list_symbols(struct archive *a);
void *ar_find_file(struct archive *a, const char *name);
void *ar_find_symbol(struct archive *a, const char *name);
void ar_free(struct archive *a);
int ar_load(char *p, unsigned long size, struct archive *a);




#endif /* _KERNEL_AR_H_ */

