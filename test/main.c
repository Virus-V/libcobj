#include <sys/types.h>

#include <stdlib.h>
#include <sysexits.h>

#include <libcobj.h>
#include "foo_if.h"

/*
 * Generic class implements common method.
 */ 
static cobj_method_t null_methods[] = {
	COBJ_METHOD_END
};

DEFINE_CLASS(null, null_methods, sizeof(struct cobj));

/*
 * Generic class implements specific methods.
 */
static void
own_method(cobj_t o)
{
	(void)printf("%s: instance of %s_class \n", 
		__func__, o->ops->cls->name);
}

static void
own_static_method(cobj_t o)
{
	cobj_class_t c;
	
	c = (cobj_class_t)o;
	
	(void)printf("%s: of %s_class \n", __func__, c->name);
}

static cobj_method_t foo_methods[] = {
	COBJ_METHOD(foo_bar,			own_method),
	COBJ_METHOD(foo_static_bar,		own_static_method),
	COBJ_METHOD_END
};

DEFINE_CLASS(foo, foo_methods, sizeof(struct cobj));

int
main(int argc, char *argv[])
{
	cobj_t o;
	
	/*
	 * Initialize foo_class(3) and call its 
	 * statically defined method, if any.
	 */
	cobj_class_compile(&foo_class);
	FOO_STATIC_BAR(&foo_class);
	
	/* 
	 * Allocate and map its class and call 
	 * its default implementation for the
	 * foo_bar(3) method.
	 */
	o = cobj_create(&null_class);
	FOO_BAR(o);
	
	/* 
	 * Change its type and call the specific
	 * implementation of foo_bar(3) method.
	 */
	cobj_class_free(&null_class);
	cobj_init(o, &foo_class);
	FOO_BAR(o);
	
	/* 
	 * Try to call a not implemented method. 
	 */
	FOO_BAZ(o, 1);
	
	exit(EX_OK);
}
