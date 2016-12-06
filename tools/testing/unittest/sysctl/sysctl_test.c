#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <kselftest.h>

#include <shared.h>
#include <malloc_test_wrapper.h>

/* include header + src file for static function testing */
#include <kernel/sysctl.h>
#include <sysctl.c>


static ssize_t _show(__attribute__((unused)) struct sysobj *sobj,
		     __attribute__((unused)) struct sobj_attribute *sattr,
		     __attribute__((unused)) char *buf)
{
	buf[0] = '\0';

	return 0;
}

static ssize_t _store(__attribute__((unused)) struct sysobj *sobj,
		      __attribute__((unused)) struct sobj_attribute *sattr,
		      __attribute__((unused)) const char *buf,
		      __attribute__((unused)) size_t len)
{
	return 0;
}




/**
 * @test sysobject name
 */

static void sysobj_name_test(void)
{
	struct sysobj sobj;

	sobj.name = "test";

	/* function returns pointer to object name */
	KSFT_ASSERT(sobj.name == sysobj_name(&sobj));
}


/**
 * @test sobj_sysset_join
 */

static void sobj_sysset_join_test(void)
{
	struct sysobj *sobj;
	struct sysset *sset;


	sobj = sysobj_create();

	bzero(sobj, sizeof(struct sysobj));

	/* uninitialised */
	sobj_sysset_join(sobj);


	sset = sysset_create_and_add("set", NULL, NULL);
	sysobj_add(sobj, NULL, sset, "obj");

	/* initialised */
	sobj_sysset_join(sobj);



	free(sset);
	free(sobj);
}


/**
 * @test sysobj_get_test
 */

static void sysobj_get_test(void)
{
	struct sysobj sobj;

	KSFT_ASSERT(&sobj == sysobj_get(&sobj));
}


/**
 * @test sysobj_set_name
 */

static void sysobj_set_name_test(void)
{
	struct sysobj sobj;


	sysobj_set_name(&sobj, "test");

	KSFT_ASSERT(strcmp("test", sobj.name) == 0);
}


/**
 * @test sysobj_init_internal
 */

static void sysobj_init_internal_test(void)
{
	struct sysobj sobj;


	sysobj_init_internal(NULL);

	sobj.entry.prev = NULL;
	sobj.entry.next = NULL;

	sysobj_init_internal(&sobj);

	KSFT_ASSERT(&sobj.entry == sobj.entry.prev);
	KSFT_ASSERT(&sobj.entry == sobj.entry.next);
}


/**
 * @test sysobj_init
 */

static void sysobj_init_test(void)
{
	struct sysobj sobj;


	sysobj_init(NULL);

	sobj.entry.prev = NULL;
	sobj.entry.next = NULL;

	sysobj_init(&sobj);

	KSFT_ASSERT(&sobj.entry == sobj.entry.prev);
	KSFT_ASSERT(&sobj.entry == sobj.entry.next);
}


/**
 * @test sysobj_create
 */

static void sysobj_create_test(void)
{
	struct sysobj *sobj;



	malloc_test_wrapper(ENABLED);

	sobj = sysobj_create();
	KSFT_ASSERT_PTR_NULL(sobj);

	malloc_test_wrapper(DISABLED);

	sobj = sysobj_create();
	KSFT_ASSERT(&sobj->entry == sobj->entry.prev);
	KSFT_ASSERT(&sobj->entry == sobj->entry.next);

	free(sobj);
}


/**
 * @test sysobj_add_internal
 */

static void sysobj_add_internal_test(void)
{
	struct sysobj *sobj;


	KSFT_ASSERT(sysobj_add_internal(NULL) == -1);

	sobj = sysobj_create();

	KSFT_ASSERT(sysobj_add_internal(sobj) == 0);

	sobj->parent = NULL;
	KSFT_ASSERT(sysobj_add_internal(sobj) == 0);

	sobj->sysset = sysset_create_and_add("test", NULL, NULL);
	KSFT_ASSERT(sysobj_add_internal(sobj) == 0);

	sobj->parent = sobj;
	KSFT_ASSERT(sysobj_add_internal(sobj) == 0);

	free(sobj->sysset);
	free(sobj);
}


/**
 * @test sysobj_add
 */

static void sysobj_add_test(void)
{
	struct sysobj *sobj = NULL;


	KSFT_ASSERT(sysobj_add(sobj, NULL, NULL, NULL) == -1);

	sobj = sysobj_create();

	KSFT_ASSERT(sysobj_add(sobj, NULL, NULL, NULL) == 0);

	free(sobj);
}


/**
 * @test sysobj_create_and_add
 */

static void sysobj_create_and_add_test(void)
{
	struct sysobj *sobj = NULL;


	malloc_test_wrapper(ENABLED);

	KSFT_ASSERT_PTR_NULL(sysobj_create_and_add("test", NULL));

	malloc_test_wrapper(DISABLED);


	sobj = sysobj_create_and_add("test", NULL);
	KSFT_ASSERT_PTR_NOT_NULL(sobj);

	free(sobj);
}


/**
 * @test sysobj_list_attr
 */
__extension__
static void sysobj_list_attr_test(void)
{
	struct sysobj sobj;
	struct sobj_attribute sattr = __ATTR(test, NULL, NULL);
	struct sobj_attribute *attr[] = {&sattr, NULL};

	sysobj_list_attr(NULL);

	sobj.sattr = NULL;

	sysobj_list_attr(&sobj);

	sobj.sattr = attr;

	sysobj_list_attr(&sobj);

}


/**
 * @test sysobj_show_attr
 */
__extension__
static void sysobj_show_attr_test(void)
{
	struct sysobj sobj;
	struct sobj_attribute sattr = __ATTR(test, _show, _store);
	struct sobj_attribute sattr2 = __ATTR(nomatch, _show, _store);
	struct sobj_attribute *attr[] = {&sattr, &sattr2, NULL};

	char buf[256];


	sysobj_show_attr(NULL, NULL, buf);

	sysobj_show_attr(NULL, "test", buf);

	sobj.sattr = NULL;

	sysobj_show_attr(&sobj, "test", buf);

	sobj.sattr = attr;

	sysobj_show_attr(&sobj, "test", buf);
}


/**
 * @test sysobj_store_attr
 */
__extension__
static void sysobj_store_attr_test(void)
{
	struct sysobj sobj;
	struct sobj_attribute sattr = __ATTR(test, _show, _store);
	struct sobj_attribute sattr2 = __ATTR(nomatch, _show, _store);
	struct sobj_attribute *attr[] = {&sattr, &sattr2, NULL};

	sysobj_store_attr(NULL, NULL, NULL, 0);

	sysobj_store_attr(NULL, "test", NULL, 0);

	sobj.sattr = NULL;

	sysobj_store_attr(&sobj, "test", NULL, 0);

	sobj.sattr = attr;

	sysobj_store_attr(&sobj, "test", NULL, 0);
}


/**
 * @test sysset_register
 */
static void sysset_register_test(void)
{
	struct sysset *sset;


	sset = sysset_create("test", NULL, NULL);

	KSFT_ASSERT(sysset_register(NULL) == -1);

	KSFT_ASSERT(sysset_register(sset) == 0);


	free(sset);
}


/**
 * @test to_sysset
 */

static void to_sysset_test(void)
{
	struct sysset *sset;


	sset = sysset_create("test", NULL, NULL);

	KSFT_ASSERT_PTR_NULL(to_sysset(NULL));
	KSFT_ASSERT_PTR_NOT_NULL(to_sysset(&sset->sobj));

	free(sset);
}


/**
 * @test sysset_get
 */

static void sysset_get_test(void)
{
	struct sysset *sset;


	sset = sysset_create("test", NULL, NULL);

	KSFT_ASSERT_PTR_NULL(sysset_get(NULL));
	KSFT_ASSERT_PTR_NOT_NULL(sysset_get(sset));

	free(sset);

}


/*
 * @test sysset_create
 */

static void sysset_create_test(void)
{
	struct sysset *sset;


	malloc_test_wrapper(ENABLED);
	KSFT_ASSERT_PTR_NULL(sysset_create("test", NULL, NULL));
	malloc_test_wrapper(DISABLED);

	sset = sysset_create("test", NULL, NULL);
	KSFT_ASSERT_PTR_NOT_NULL(sset);

	free(sset);
}


/*
 * @test sysset_create_and_add
 */
static void sysset_create_and_add_test(void)
{
	struct sysset *sset;


	malloc_test_wrapper(ENABLED);
	KSFT_ASSERT_PTR_NULL(sysset_create_and_add("test", NULL, NULL));
	malloc_test_wrapper(DISABLED);

	sset = sysset_create_and_add("test", NULL, NULL);
	KSFT_ASSERT_PTR_NOT_NULL(sset);

	free(sset);
}


/*
 * @test sysset_find_obj
 */
static void sysset_find_obj_test(void)
{
	struct sysobj *sobj[2];

	struct sysset *sset[3];


	sset[0] = sysset_create_and_add("test", NULL, NULL);
	sset[1] = sysset_create_and_add("test2", NULL, sset[0]);
	sset[2] = sysset_create_and_add(NULL, NULL, sset[1]);

	sobj[0] = sysobj_create();
	sysobj_add(sobj[0], NULL, sset[0], "blah");

	sobj[1] = sysobj_create();
	sysobj_add(sobj[1], NULL, sset[1], "blah2");

	sysset_find_obj(sset[0], "/nonexistent");
	sysset_find_obj(sset[0], "/test/nonexistent");
	sysset_find_obj(sset[0], "/test/blah");
	sysset_find_obj(sset[0], "/test/test2");
	sysset_find_obj(sset[0], "/test/test2/faux");



	free(sset[0]);
	free(sset[1]);
	free(sset[2]);

	free(sobj[0]);
	free(sobj[1]);
}


/*
 * @test sysset_show_tree
 */
static void sysset_show_tree_test(void)
{
	struct sysobj *sobj[2];

	struct sysset *sset[3];


	sset[0] = sysset_create_and_add("test", NULL, NULL);
	sset[1] = sysset_create_and_add("test2", NULL, sset[0]);
	sset[2] = sysset_create_and_add(NULL, NULL, sset[1]);

	sobj[0] = sysobj_create();
	sysobj_add(sobj[0], NULL, sset[0], "blah");

	sobj[1] = sysobj_create();
	sysobj_add(sobj[1], NULL, sset[1], "blah2");

	sysset_show_tree(sset[0]);


	free(sset[0]);
	free(sset[1]);
	free(sset[2]);

	free(sobj[0]);
	free(sobj[1]);
}


/*
 * @test sysctl_init_test
 */
static void sysctl_init_test(void)
{

	/* TODO not fully covered, needs 3x failing malloc() */
#if 0
	malloc_test_wrapper(ENABLED);
	KSFT_ASSERT(sysctl_init() == -1);
	KSFT_ASSERT(sysctl_init() == -1);
	KSFT_ASSERT(sysctl_init() == -1);
#endif

	KSFT_ASSERT(sysctl_init() ==  0);

	/* internals */
	free(driver_set);
	free(sys_set);
}


int main(int argc, char **argv)
{

	printf("Testing sysctl interface\n\n");

	KSFT_RUN_TEST("sysobj name",
	 	   sysobj_name_test);

	KSFT_RUN_TEST("sobj sysset join",
	 	   sobj_sysset_join_test);

	KSFT_RUN_TEST("sobj get",
	 	   sysobj_get_test);

	KSFT_RUN_TEST("sysobj set name",
	 	   sysobj_set_name_test);

	KSFT_RUN_TEST("sysobj init internal",
	 	   sysobj_init_internal_test);

	KSFT_RUN_TEST("sysobj init",
	 	   sysobj_init_test);

	KSFT_RUN_TEST("sysobj create",
	 	   sysobj_create_test);

	KSFT_RUN_TEST("sysobj add internal",
	 	   sysobj_add_internal_test);

	KSFT_RUN_TEST("sysobj add",
	 	   sysobj_add_test);

	KSFT_RUN_TEST("sysobj create and add",
	 	   sysobj_create_and_add_test);

	KSFT_RUN_TEST("sysobj list attr",
	 	   sysobj_list_attr_test);

	KSFT_RUN_TEST("sysobj show attr",
	 	   sysobj_show_attr_test);

	KSFT_RUN_TEST("sysobj store attr",
	 	   sysobj_store_attr_test);

	KSFT_RUN_TEST("sysset register",
	 	   sysset_register_test);

	KSFT_RUN_TEST("to sysset",
	 	   to_sysset_test);

	KSFT_RUN_TEST("sysset get",
	 	   sysset_get_test);

	KSFT_RUN_TEST("sysset create",
	 	   sysset_create_test);

	KSFT_RUN_TEST("sysset create",
	 	   sysset_create_and_add_test);

	KSFT_RUN_TEST("sysset find obj",
	 	   sysset_find_obj_test);

	KSFT_RUN_TEST("sysset show tree",
		   sysset_show_tree_test);

	KSFT_RUN_TEST("sysctl init",
		   sysctl_init_test);


	printf("syctl interface test complete:\n");

	ksft_print_cnts();

	return ksft_exit_pass();
}
