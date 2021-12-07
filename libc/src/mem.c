/**
 * adapted from kernel/mem.c
 */

#include <list.h>
#include <syscall.h>
#include <stddef.h>

void *malloc(size_t size);
void *zalloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/**
 * XXX here we must initiate FDIR procedures instead of looping
 *     OR alternatively do nothing and let the OS deal with the fallout
 */
#define panic(x) {} while(1);
#define BUG() do { \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        panic("BUG!"); \
} while (0)

#define ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)		ALIGN_MASK(x, (typeof(x))(a) - 1)
#define WORD_ALIGN(x)		ALIGN((x), sizeof(unsigned long))



#ifdef CONFIG_MMU
/* sbrk syscall here */
static void *sbrk(int x);
#else

/* mem_alloc syscall for non-mmu systems */
static void *sys_mem_alloc(size_t size)
{
	void *p = NULL;

	SYSCALL2(2, size, &p);

	return p;
}

static void sys_mem_free(void *p)
{
	SYSCALL1(3, p);
}

#endif /* CONFIG_MMU */


#define WORD_ALIGN(x)	ALIGN((x), sizeof(unsigned long))

#ifdef CONFIG_MMU
struct mem {
	void *data;
	size_t size;
	int free;
	struct mem *prev, *next;
	struct list_head node;
} __attribute__((aligned));

/* the initial and the most recent allocation */
static struct mem *_mem_init;
static struct mem *_mem_last;


/**
 * @brief see if we can find a suitable chunk in our pool
 */

static struct mem *mem_find_free_chunk(size_t size, struct mem **prev)
{
	struct mem *p_tmp;
	struct mem *p_elem;

	(*prev) = _mem_last;

	if (list_empty(&_mem_init->node))
		return NULL;

	list_for_each_entry_safe(p_elem, p_tmp, &_mem_init->node, node) {
		if (!p_elem->free)
			BUG();

		if (p_elem->size >= size) {
			(*prev) = p_elem->prev;
			return p_elem;
		}
	}


	return NULL;
}


/**
 * @brief split a chunk in two for a given size
 */

static void mem_split(struct mem *m, size_t size)
{
	struct mem *split;


	split = (struct mem *)((size_t) m + size);


	split->free = 1;
	split->data = split + 1;

	split->prev = m;
	split->next = m->next;

	split->size = m->size - size;

	m->size = size - sizeof(struct mem);

	if (m->next)
		m->next->prev = split;

	m->next = split;

	list_add_tail(&split->node, &_mem_init->node);
}


/**
 * @brief merge a chunk with its neighbour
 */

static void mem_merge(struct mem *m)
{
	m->size = m->size + m->next->size + sizeof(struct mem);

	m->next = m->next->next;

	if (m->next)
		m->next->prev = m;
}
#endif /* CONFIG_MMU */


/**
 * @brief returns the initial mem chunk
 *
 * @returns pointer or NULL on error
 */

void *mem_init(void)
{
#ifdef CONFIG_MMU
	if (likely(_mem_init))
		return _mem_init;

	_mem_init = sbrk(WORD_ALIGN(sizeof(struct mem)));

	if (_mem_init == (void *) -1) {
		printf("MEM: error, cannot _mem_initialise\n");
		return NULL;
	}

	_mem_init->data = NULL;
	_mem_init->size = 0;
	_mem_init->free = 0;
	_mem_init->prev = NULL;
	_mem_init->next = NULL;

	/* we track our free chunks in the node of the initial allocation */
	INIT_LIST_HEAD(&_mem_init->node);

	_mem_last = _mem_init;

	return _mem_init;
#else
	return (void *) -1; /* return something not NULL */
#endif
}


/**
 * @brief allocates size bytes and returns a pointer to the allocated memory,
 *	  suitably aligned for any built-in type
 *
 * @param size the number of bytes to allocate
 *
 * @returns a pointer or NULL on error or size == 0
 */

void *malloc(size_t size)
{
#ifdef CONFIG_MMU
	size_t len;

	struct mem *m_new;
	struct mem *m_prev = NULL;


	if (!size)
		return NULL;

	/* make sure we are initialised, this is a cheap call */
	mem_init();

	len = WORD_ALIGN(size + sizeof(struct mem));

	/* try to locate a free chunk first */
	m_new = mem_find_free_chunk(len, &m_prev);

	if (m_new) {
		/* take only what we need */
		if ((len + sizeof(struct mem)) < m_new->size)
			mem_split(m_new, len);

		m_new->free = 0;

		return m_new->data;
	}


	/* need fresh memory */
	m_new = sbrk(len);

	if (m_new == (void *) -1)
		return NULL;

	m_new->free = 0;

	m_new->next = NULL;

	/* link */
	m_new->prev = m_prev;
	m_prev->next = m_new;

	m_new->size = len - sizeof(struct mem);

	/* data section follows just after */
	m_new->data = m_new + 1;

	_mem_last = m_new;

	return m_new->data;
#else
	return sys_mem_alloc(size);
#endif /* CONFIG_MMU */

}


/**
 * @brief allocates memory for an array of nmemb elements of size bytes each and
 *	   returns a pointer to the allocated memory. The memory is set to zero.
 *
 * @param nmemb the number of elements
 * @param size the number of bytes per element
 *
 * @returns a pointer or NULL on error or nmemb/size == 0
 */

void *calloc(size_t nmemb, size_t size)
{
	size_t i;
	size_t len;

	char *dst;
	void *ptr;


	len = nmemb * size;

	ptr = malloc(len);

	if (ptr) {
		dst = ptr;
		for (i = 0; i < len; i++)
			dst[i] = 0;
	}

	return ptr;
}


/**
 * @brief allocates size bytes and returns a pointer to the allocated memory,
 *	  suitably aligned for any built-in type. The memory is set to zero.
 *
 * @param size the number of bytes to allocate
 *
 * @returns a pointer or NULL on error or size == 0
 *
 * @note this should be preferred over calloc(n, 1), as it saves the extra
 *	 argument and hence produces less code
 */

void *zalloc(size_t size)
{
	return calloc(size, 1);
}


/**
 * @brief changes the size of the memory block pointed to by ptr to size bytes.
 *	  The contents will be unchanged in the range from the start of the
 *	  region up to the minimum of the old and new sizes. If the new size is
 *	  larger than the old size,the added memory will not be initialized.
 *
 * @param ptr the old memory block, if NULL, this function is equal to malloc()
 * @param size the number of bytes for the new block, if 0, this is equal to
 *	  free()
 *
 * @returns a pointer or NULL on error or size == 0
 */

void *realloc(void *ptr, size_t size)
{
	size_t i;

	size_t len;

	char *dst;
	char *src;

	void *ptr_new;

#ifdef CONFIG_MMU
	struct mem *m;
#endif /* CONFIG_MMU */

	if (!ptr)
		return malloc(size);

#ifdef CONFIG_MMU
	if (ptr < mem_init())
		return NULL;

	if (ptr > sbrk(0))
		return NULL;


	m = ((struct mem *) ptr) - 1;

	if (m->data != ptr)
		return NULL;

#endif /* CONFIG_MMU */

	ptr_new = malloc(size);

	if (!ptr_new)
		return NULL;

#ifdef CONFIG_MMU
	if (m->size > size)
		len = size;
	else
		len = m->size;
#else
	/* this WILL copy out of bounds if size < original */
	len = size;
#endif /* CONFIG_MMU */

	src = ptr;
	dst = ptr_new;

	for (i = 0; i < len; i++)
		dst[i] = src[i];

	free(ptr);

	return ptr_new;
}


/**
 * @brief function frees the memory space pointed to by ptr, which must have
 *	  been returned by a previous call to malloc(), calloc(),
 *	  or krealloc()
 *
 * @param ptr the memory to free
 */

void free(void *ptr)
{
#ifdef CONFIG_MMU
	struct mem *m;


	if (!ptr)
		return;

	if (ptr < mem_init())
		return;

	if (ptr > sbrk(0))
		return;


	m = ((struct mem *) ptr) - 1;

	if (m->data != ptr)
		return;

	m->free = 1;

	if (m->next && m->next->free) {
		mem_merge(m);
	} else if (m->prev->free) {
		m = m->prev;
		mem_merge(m);
	}

	if (!m->next) {
		m->prev->next = NULL;
		_mem_last = m->prev;

		/* release back */
		sbrk(-(m->size + sizeof(struct mem)));
	} else {
		list_add_tail(&m->node, &_mem_init->node);
	}
#else
	sys_mem_free(ptr);
#endif /* CONFIG_MMU */
}
