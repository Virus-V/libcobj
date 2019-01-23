/*-
 * Copyright (c) 2019 Henning Matyschok
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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

/*
 * Import tailq_class(3).
 */
#include "tailq_if.h" 

DECLARE_CLASS(tailq_class);

int item_a = 1;
int item_b = 2;
int item_c = 3;
int *tmp;

int
main(int argc, char *argv[])
{
	cobj_t o;
	
	/*
	 * Initialize foo_class(3) and call its 
	 * statically defined method, if any.
	 */
	(void)cobj_class_compile(&foo_class);
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
	(void)cobj_class_free(&null_class);
	(void)cobj_init(o, &foo_class);
	FOO_BAR(o);
	
	/* 
	 * Try to call a not implemented method. 
	 */
	FOO_BAZ(o, 1);
	(void)cobj_delete(o);
	
	/*
	 * Initialize tailq_class(3), because 
	 * tailq_{create,destroy}(3) are static 
	 * class methods.
	 */
	(void)cobj_class_compile(&tailq_class);
	
	/*
	 * Create an instance of tailq_class(3).
	 */
	o = TAILQ_CREATE(&tailq_class);
	
	/*
	 * Enqueue.
	 */
	(void)TAILQ_ADD(o, &item_a);
	(void)TAILQ_ADD(o, &item_b);
	(void)TAILQ_ADD(o, &item_c);
	
	/*
	 * Dequeue.
	 */
	while ((tmp = (int *)TAILQ_POLL(o)) != NULL)
		(void)printf("%s: item: %d\n", __func__, *tmp);
	
	/*
	 * Destroy instance of tailq_class(3).
	 */
	(void)TAILQ_DESTROY(&tailq_class, o);
	
	exit(EX_OK);
}
