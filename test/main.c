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
 * Generic class implements specific method.
 */
static void
own_method(cobj_t o)
{
	(void)printf("%s: ... \n", __func__);
}

static cobj_method_t foo_methods[] = {
	COBJ_METHOD(foo_bar,	own_method),
	COBJ_METHOD_END
};

DEFINE_CLASS(foo, foo_methods, sizeof(struct cobj));

int
main(int argc, char *argv[])
{
	cobj_t o;
	
	o = cobj_create(&null_class);
	FOO_BAR(o);
	
	cobj_class_free(&null_class);
	cobj_init(o, &foo_class);
	FOO_BAR(o);
	FOO_BAZ(o, 1);
	
	exit(EX_OK);
}
