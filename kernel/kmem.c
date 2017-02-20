/**
 * @file kernel/kmem.c
 *
 * A simple high-level allocator that uses sbrk() to retrieve memory.
 * Is similar to lib/chunk.c, but a lot simpler. I need to respect sbrk()
 * so I can't just use _chunk_ because it may release any parent chunk,
 * while we need to do that from _kmem_last  to first as the become free.
 * I don't have time do do this properly, so this must do for now..
 */

#include <list.h>
#include <kernel/kmem.h>
#include <kernel/sbrk.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>


#define WORD_ALIGN(x)	ALIGN((x), (sizeof(unsigned long) - 1))


struct kmem {
	void *data;
	size_t size;
	int free;
	struct kmem *prev, *next;
	struct list_head node;
} __attribute__((aligned));

/* the initial and the most recent allocation */
static struct kmem *_kmem_init;
static struct kmem *_kmem_last;


/**
 * @brief see if we can find a suitable chunk in our pool
 */

static struct kmem *kmem_find_free_chunk(size_t size, struct kmem **prev)
{
	struct kmem *p_tmp;
	struct kmem *p_elem;

	(*prev) = _kmem_last;

	if (list_empty(&_kmem_init->node))
		return NULL;

	list_for_each_entry_safe(p_elem, p_tmp, &_kmem_init->node, node) {
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

static void kmem_split(struct kmem *k, size_t size)
{
	struct kmem *split;


	split = (struct kmem *)((size_t) k + size);


	split->free = 1;
	split->data = split + 1;

	split->prev = k;
	split->next = k->next;

	split->size = k->size - size;

	k->size = size - sizeof(struct kmem);

	if (k->next)
		k->next->prev = split;

	k->next = split;

	list_add_tail(&split->node, &_kmem_init->node);
}



/**
 * @brief returns the initial kmem chunk
 *
 * @note call this once kernel_sbrk() works
 */

void *kmem_init(void)
{
	if (likely(_kmem_init))
		return _kmem_init;

	_kmem_init = kernel_sbrk(WORD_ALIGN(sizeof(struct kmem)));

	if (_kmem_init == (void *) -1) {
		pr_crit("KMEM: error, cannot _kmem_initialise\n");
		return NULL;
	}

	_kmem_init->data = NULL;
	_kmem_init->size = 0;
	_kmem_init->free = 0;
	_kmem_init->prev = NULL;
	_kmem_init->next = NULL;

	/* we track our free chunks in the node of the initial allocation */
	INIT_LIST_HEAD(&_kmem_init->node);

	_kmem_last = _kmem_init;

	return _kmem_init;
}



/**
 * @brief merge a chunk with its neighbour
 */

static void kmem_merge(struct kmem *k)
{
	k->size = k->size + k->next->size + sizeof(struct kmem);

	k->next = k->next->next;

	if (k->next)
		k->next->prev = k;
}


/**
 * @brief allocates size bytes and returns a pointer to the allocated memory,
 *	  suitably aligned for any built-in type
 *
 * @param size the number of bytes to allocate
 *
 * @returns a pointer or NULL on error or size == 0
 */

void *kmalloc(size_t size)
{
	size_t len;

	struct kmem *k_new;
	struct kmem *k_prev = NULL;


	if (!size)
		return NULL;

	len = WORD_ALIGN(size + sizeof(struct kmem));

	/* try to locate a free chunk first */
	k_new = kmem_find_free_chunk(size, &k_prev);

	if (k_new) {
		/* take only what we need */
		if ((len + sizeof(struct kmem)) < k_new->size)
			kmem_split(k_new, len);

		k_new->free = 0;

		return k_new->data;
	}


	/* need fresh memory */
	k_new = kernel_sbrk(len);

	if (k_new == (void *) -1)
		return NULL;

	k_new->free = 0;

	k_new->next = NULL;

	/* link */
	k_new->prev = k_prev;
	k_prev->next = k_new;

	k_new->size = len - sizeof(struct kmem);

	/* data section follows just after */
	k_new->data = k_new + 1;

	_kmem_last = k_new;

	return k_new->data;
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

void *kcalloc(size_t nmemb, size_t size)
{
	size_t i;
	size_t len;

	char *dst;
	void *ptr;


	len = nmemb * size;

	ptr = kmalloc(len);

	if (ptr) {
		dst = ptr;
		for (i = 0; i < len; i++)
			dst[i] = 0;
	}

	return ptr;
}


/**
 * @brief changes the size of the memory block pointed to by ptr to size bytes.
 *	  The contents will be unchanged in the range from the start of the
 *	  region up to the minimum of the old and new sizes. If the new size is
 *	  larger than the old size,the added memory will not be initialized.
 *
 * @param ptr the old memory block, if NULL, this function is equal to kmalloc()
 * @param size the number of bytes for the new block, if 0, this is equal to
 *	  kfree()
 *
 * @returns a pointer or NULL on error or size == 0
 */

void *krealloc(void *ptr, size_t size)
{
	size_t i;

	size_t len;

	char *dst;
	char *src;

	void *ptr_new;
	struct kmem *k;



	if (!ptr)
		return kmalloc(size);

	if (ptr < kmem_init()) {
		pr_warning("KMEM: invalid krealloc() of addr %p below lower "
			   "bound of trackable memory in call from %p\n",
			   ptr, __caller(0));
		return NULL;
	}

	if (ptr > kernel_sbrk(0)) {
		pr_warning("KMEM: invalid krealloc() of addr %p beyond system "
			   "break in call from %p\n",
			   ptr, __caller(0));
		return NULL;
	}


	k = ((struct kmem *) ptr) - 1;

	if (k->data != ptr) {
		pr_warning("KMEM: invalid krealloc() of addr %p in call "
			   "from %p\n",
			   ptr, __caller(0));
		return NULL;
	}


	ptr_new = kmalloc(size);

	if (!ptr_new)
		return NULL;


	if (k->size > size)
		len = size;
	else
		len = k->size;

	src = ptr;
	dst = ptr_new;

	for (i = 0; i < len; i++)
		dst[i] = src[i];

	kfree(ptr);

	return ptr_new;
}


/**
 * @brief function frees the memory space pointed to by ptr, which must have
 *	  been returned by a previous call to kmalloc(), kcalloc(),
 *	  or krealloc()
 *
 * @param ptr the memory to free
 */

void kfree(void *ptr)
{
	struct kmem *k;


	if (!ptr)
		return;

	if (ptr < kmem_init()) {
		pr_warning("KMEM: invalid kfree() of addr %p below lower bound "
			   "of trackable memory in call from %p\n",
			   ptr, __caller(0));
		return;
	}

	if (ptr > kernel_sbrk(0)) {
		pr_warning("KMEM: invalid kfree() of addr %p beyond system "
			   "break in call from %p\n",
			   ptr, __caller(0));
		return;
	}


	k = ((struct kmem *) ptr) - 1;

	if (k->data != ptr) {
		pr_warning("KMEM: invalid kfree() of addr %p in call from %p\n",
			   ptr, __caller(0));
		return;
	}

	k->free = 1;

	if (k->next && k->next->free)
		kmem_merge(k);

	if (k->prev->free) {
		k = k->prev;
		kmem_merge(k);
	}

	if (!k->next) {
		k->prev->next = NULL;
		_kmem_last = k->prev;

		/* release back */
		kernel_sbrk(-(k->size + sizeof(struct kmem)));
	} else {
		list_add_tail(&k->node, &_kmem_init->node);
	}
}
