/**
 *
 * This is a very simple AR reader. It expects indexed SVR4/GNU type archive,
 * i.e. created invoking: ar rcs <archive> <files...>
 * in the following format:
 *
 * !<arch><ar header><SVR4/GNU symbol table><entries...>
 *
 * Note that there are only basic checks for actual file size. This really
 * depends on uncorrupted files, so you might want to verify them ahead of
 * calling this.
 *
 *
 * @note this does produce references to the parsed file, rather than create a
 *       copy
 *
 * @note The sym array in struct archive may be used to lookup a module
 *       dependency when resolving symbols.
 */



#include <stdlib.h>	/* strtol() */
#include <string.h>	/* strncmp() */

#include <kernel/ar.h>
#include <kernel/kmem.h>
#include <kernel/printk.h>



/* XXX cleanup*/
#define PLATFORM_IS_LITTLE_ENDIAN 0
#if PLATFORM_IS_LITTLE_ENDIAN
/**
 * @brief byte swap unsigned int
 */

static uint32_t swap_uint32(uint32_t x)
{
	x = ((x << 8) & 0xFF00FF00UL) | ((x >> 8) & 0x00FF00FFUL);

	return (x << 16) | (x >> 16);
}
#endif

/**
 * @brief verify ar file header magic
 *
 * @param p a buffer to the start of the file buffer
 *
 * @return pointer to first header, NULL if no match
 */

static char *ar_check_magic(char *p)
{
	if (strncmp(p, AR_MAG, strlen(AR_MAG)))
		return NULL;

	return p + strlen(AR_MAG);
}


/**
 * @brief reads an ar header
 *
 * @param p a pointer to the next header record
 * @param h a struct ar_hdr
 *
 * @return pointer to next record
 */

static char *ar_read_hdr(char *p, struct ar_hdr *hdr)
{
	memcpy(hdr, p, sizeof(struct ar_hdr));

	return p + sizeof(struct ar_hdr);
}

/**
 * @brief locate the associated string table entry for an oversized file name
 *
 * @param a a struct archive
 * @param hdr a struct ar_hdr
 *
 * @return the pointer to the string entry or NULL if not found
 */

static char *ar_get_strtbl_filename(struct archive *a, struct ar_hdr *hdr)
{
	long off;

	char *str;


	off =  strtol(&hdr->ar_name[1], NULL, 10);


	str = (char *) a->ar_str + off;

	if (str > (a->ar_base + a->ar_size))
		str = NULL;

	if (str < a->ar_base)
		str = NULL;

	return str;
}


/**
 * @brief get string table reference
 *
 * @param a
 *
 * @return the converted value
 */

static long int ar_get_entry_size(struct ar_hdr *hdr)
{
	return strtol(hdr->ar_size, NULL, sizeof(hdr->ar_size));
}


/**
 * @brief process a SVR4/GNU archive symbol table
 *
 * @param p a pointer to the next record
 * @param a a struct archive
 * @param hdr a struct ar_hdr
 *
 * @return pointer to next record or NULL on error
 */

/* XXX: #define PLATFORM_IS_LITTLE_ENDIAN 1 */
static char *ar_process_sym_tbl(char *p, struct archive *a, struct ar_hdr *hdr)
{
	unsigned long i, j;

	long int recsize;
	long int strsize;

	uint32_t *symoff;


	if (a->n_sym) {
		pr_err("AR: Error, more than one symbol table in file.\n");
		return NULL;
	}


	recsize = ar_get_entry_size(hdr);

	if ((p + recsize)  > (a->ar_base + a->ar_size)) {
		pr_err("AR: Error, symbol table header size is foul.\n");
		return NULL;
	}

	pr_debug("AR: Symbol table entry size is %d\n", recsize);

	/* the symbol count is 4 bytes wide, MSB first */
	memcpy(&a->n_sym, p, sizeof(uint32_t));
#if PLATFORM_IS_LITTLE_ENDIAN
	a->n_sym = swap_uint32(a->n_sym);
#endif


	p += sizeof(a->n_sym);	/* move to symbol offsets */


	a->p_sym = (typeof(a->p_sym)) kmalloc(a->n_sym * sizeof(a->p_sym));

	if (!a->p_sym) {
		pr_err("AR: Error allocating symbol start array\n");
		goto error;
	}


	/* symbol offset array entries are 4 bytes wide, MSB first */
	symoff = (uint32_t *) p;

	/* if the array is not word-aligned, we are boned... */
	/* XXX: not needed as long as we can handle unaligned access traps */
	if (((long) symoff) & (sizeof(uint32_t) - 1)) {
		pr_err("AR: Error, symbol offset array not aligned\n");
		goto error;
	}

	/* rather than the entry header, set the symbol start pointers to the
	 * beginning of the file they are in
	 */
	for (i = 0; i < a->n_sym; i++) {
#if PLATFORM_IS_LITTLE_ENDIAN
		a->p_sym[i] = (void *) a->ar_base + swap_uint32(symoff[i])
			+ sizeof(struct ar_hdr);
#else
		a->p_sym[i] = (void *) a->ar_base + symoff[i]
			+ sizeof(struct ar_hdr);
#endif
	}


	p += a->n_sym * sizeof(uint32_t);	/* move to symbol strings */


	/* size of string section is
	 * record size - symbol count entry - symbol offsets array
	 */
	strsize = recsize - sizeof(uint32_t) - (a->n_sym * sizeof(uint32_t));

	pr_debug("AR: Symbol string table size is %d\n", strsize);

	a->sym = (typeof(a->sym)) kmalloc(a->n_sym * sizeof(a->sym));

	if (!a->sym) {
		pr_err("AR: Error allocating symbol reference array\n");
		goto error;
	}


	a->sym[0] = &p[0];	/* first string */

	for (i = 1, j = 1; (i < strsize) && (j < a->n_sym); i++) {
		if (p[i] == '\0')
			a->sym[j++] = &p[i + 1];
	}


	return p + strsize;

error:
	return NULL;
}


/**
 * @brief process a string table record
 *
 * @param p a pointer to the next record
 * @param a a struct archive
 * @param hdr a struct ar_hdr
 *
 * @return pointer to next record or NULL on error
 */

static char *ar_process_strtbl(char *p, struct archive *a, struct ar_hdr *hdr)
{

	if (a->ar_str) {
		pr_err("AR: Error, more than one string table in file.\n");
		return NULL;
	}

	a->ar_str = (typeof(a->ar_str)) p;

	return p + ar_get_entry_size(hdr);
}


/**
 * @brief process a file record
 *
 * @param p a pointer to the next record
 * @param a a struct archive
 * @param hdr a struct ar_hdr
 *
 * @return pointer to next record or NULL on error
 */

static char *ar_process_file(char *p, struct archive *a, struct ar_hdr *hdr)
{
	char *str;


	str = (char *) (p - sizeof(struct ar_hdr));

	/* name is an offset into the string table */
	if (str[0] == '/')
		str = ar_get_strtbl_filename(a, hdr);


	/* set file name, start and sizes */
	a->fname[a->n_file] = (typeof((*a->fname))) str;


	a->p_file[a->n_file] = (typeof((*a->p_file))) p;

	a->filesz[a->n_file] = ar_get_entry_size(hdr);

	/* all normal strings terminate with "/" */
	a->fnamesz[a->n_file] = (unsigned int) (strchr(str, AR_STR_TERM) - str);

	/* ajdust to next record */
	p += a->filesz[a->n_file];

	a->n_file++;	/* next file */

	return p;
}


/**
 * @brief process an archive header
 *
 * @param p a pointer to the next record
 * @param a a struct archive
 * @param hdr a struct ar_hdr
 *
 * @return pointer to next record or NULL on error
 */

static char *ar_process_hdr(char *p, struct archive *a, struct ar_hdr *hdr)
{
	int ret;


	/* check if record is symbol index */
	ret = strncmp(hdr->ar_name, GNU_AR_STR_TBL_MAG,
		      strlen(GNU_AR_STR_TBL_MAG));
	if (!ret) {
		p = ar_process_strtbl(p, a, hdr);
		goto exit;
	}

	/* check if record is symbol index */
	ret = strncmp(hdr->ar_name, GNU_AR_SYM_TBL_MAG,
		      strlen(GNU_AR_SYM_TBL_MAG));
	if (!ret) {
		p = ar_process_sym_tbl(p, a, hdr);
		goto exit;
	}

	/* it's a file */
	p = ar_process_file(p, a, hdr);

exit:
	return p;
}


/**
 * @brief determine an archives file count
 *
 * @param p a pointer to the next record
 * @param a a struct archive
 *
 * @return the archive file count
 */

static unsigned int ar_get_filecount(char *p, struct archive *a)
{
	int cnt = 0;
	int ret;

	struct ar_hdr hdr;


	while (p < (a->ar_base + a->ar_size)) {
		p = ar_read_hdr(p, &hdr);

		p += ar_get_entry_size(&hdr);


		/* skip string table */
		ret = strncmp(hdr.ar_name, GNU_AR_STR_TBL_MAG,
			      strlen(GNU_AR_STR_TBL_MAG));
		if (!ret)
			continue;

		/* skip symbol */
		ret = strncmp(hdr.ar_name, GNU_AR_SYM_TBL_MAG,
			      strlen(GNU_AR_SYM_TBL_MAG));
		if (!ret)
			continue;


		cnt++;
	}

	return cnt;
}


/**
 * @brief print symbols in the archive
 *
 * @param a a struct archive
 * @param name the file name to search for
 *
 * @return a pointer or NULL if not found
 */

void ar_list_symbols(struct archive *a)
{
	unsigned long i;


	if (!a)
		return;

	if (!a->fname)
		return;

	if (!a->fnamesz)
		return;

	printk("AR:\n"
	       "AR: Symbol contents of archive at %p\n"
	       "AR:\n",
	       a->ar_base);

	printk("AR:\t[NAME]\t\t\t[ADDR]\n");

	for (i = 0; i < a->n_sym; i++) {
		printk("AR:\t%s\t\t%p\n",
		       a->sym[i], a->p_sym[i]);
	}

	printk("AR:\n");
}


/**
 * @brief print files in the archive
 *
 * @param a a struct archive
 * @param name the file name to search for
 *
 * @return a pointer or NULL if not found
 */

void ar_list_files(struct archive *a)
{
	unsigned long i;


	if (!a)
		return;

	if (!a->fname)
		return;

	if (!a->fnamesz)
		return;

	printk("AR:\n"
	       "AR: File contents of archive at %p\n"
	       "AR:\n",
	       a->ar_base);

	printk("AR:\t[NAME]\t\t\t[SIZE]\t[ADDR]\n");

	for (i = 0; i < a->n_file; i++) {
		printk("AR:\t%.*s\t\t%d\t%p\n",
		       a->fnamesz[i], a->fname[i], a->filesz[i], a->p_file[i]);
	}

	printk("AR:\n");
}



/**
 * @brief return a pointer to an archive file
 *
 * @param a a struct archive
 * @param name the file name to search for
 *
 * @return a pointer or NULL if not found
 *
 * @note this silently returns the first occurence only
 */

void *ar_find_file(struct archive *a, const char *name)
{
	unsigned long i;


	if (!a)
		goto error;

	if (!a->fname)
		goto error;

	if (!a->fnamesz)
		goto error;


	for (i = 0; i < a->n_file; i++) {
		if(!strncmp(a->fname[i], name, a->fnamesz[i]))
			return a->p_file[i];
	}

error:
	return NULL;
}


/**
 * @brief return a pointer to the archive file a symbol is located in
 *
 * @param a a struct archive
 * @param name the name to search for
 *
 * @return a pointer or NULL if not found
 *
 * @note this silently returns the first occurence only
 */

void *ar_find_symbol(struct archive *a, const char *name)
{
	unsigned long i;


	if (!a)
		return NULL;

	for (i = 0; i < a->n_sym; i++) {
		if(!strncmp(a->sym[i], name, strlen(a->sym[i])))
			return a->p_sym[i];
	}

	return NULL;
}


/**
 * @brief free memory in a struct archive
 *
 * @param a a struct archive
 */

void ar_free(struct archive *a)
{
	kfree(a->fname);
	kfree(a->p_file);
	kfree(a->filesz);
	kfree(a->fnamesz);

	kfree(a->sym);
	kfree(a->p_sym);
}


/**
 * @brief load/parse an ar archive
 *
 * @param p the memory location of the ar file
 *
 * @param size the size of the archive file
 *
 * @param a a struct archive
 *
 *
 * @return 0 on success, < 0 on error
 *
 */

int ar_load(char *p, unsigned long size, struct archive *a)
{
	unsigned int n;

	struct ar_hdr hdr;


	/* initialise */
	a->ar_base = p;
	a->ar_size = size;
	a->ar_str  = NULL;
	a->n_file  = 0;
	a->n_sym   = 0;
	a->fname   = NULL;
	a->p_file  = NULL;
	a->filesz  = NULL;
	a->fnamesz = NULL;
	a->sym     = NULL;
	a->p_sym   = NULL;

	/* verify file type */
	p = ar_check_magic(p);
	if (!p)
		goto error;


	/* don't set a->n_file, we increment that as we read the records */
	n = ar_get_filecount(p, a);

	pr_debug("AR: %d files in archive\n", n);

	a->fname = (typeof(a->fname)) kmalloc(n * sizeof(typeof(a->fname)));
	if (!a->fname) {
		pr_err("AR: Error allocating fname name string references.\n");
		goto error;
	}

	a->p_file = (typeof(a->p_file)) kmalloc(n * sizeof(typeof(a->p_file)));
	if (!a->p_file) {
		pr_err("AR: Error allocating file pointer references.\n");
		goto error;
	}

	a->filesz = (typeof(a->filesz)) kmalloc(n * sizeof(typeof(a->filesz)));
	if (!a->filesz) {
		pr_err("AR: Error allocating file size references.\n");
		goto error;
	}

	a->fnamesz = (typeof(a->fnamesz)) kmalloc(n * sizeof(typeof(a->fnamesz)));
	if (!a->fnamesz) {
		pr_err("AR: Error allocating file name size references.\n");
		goto error;
	}


	while (p < (a->ar_base + a->ar_size)) {
		/* read next header record */
		p = ar_read_hdr(p, &hdr);

		p = ar_process_hdr(p, a, &hdr);
		if (!p)
			goto error;
	}


	return 0;

error:
	pr_err("AR: Error occured in ar_load()\n");
	ar_free(a);

	return -1;
}
