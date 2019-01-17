/*-
 * Copyright (c) 2000,2003 Doug Rabson
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
 *
 *	$FreeBSD: releng/11.1/sys/sys/kobj.h 318274 2017-05-14 14:21:09Z marius $
 */

#ifndef _COBJ_H_
#define _COBJ_H_

/*
 * Forward declarations
 */
typedef struct cobj		*cobj_t;
typedef struct cobj_class	*cobj_class_t;
typedef const struct cobj_method cobj_method_t;
typedef int			(*cobjop_t)(void);
typedef struct cobj_ops		*cobj_ops_t;
typedef struct cobjop_desc	*cobjop_desc_t;

struct cobj_method {
	cobjop_desc_t	desc;
	cobjop_t	func;
};

/*
 * A class is simply a method table and a sizeof value. When the first
 * instance of the class is created, the method table will be compiled 
 * into a form more suited to efficient method dispatch. This compiled 
 * method table is always the first field of the object.
 */
#define COBJ_CLASS_FIELDS						\
	const char	*name;		/* class name */		\
	cobj_method_t	*methods;	/* method table */		\
	size_t		size;		/* object size */		\
	cobj_class_t	*baseclasses;	/* base classes */		\
	u_int		refs;		/* reference count */		\
	cobj_ops_t	ops		/* compiled method table */

struct cobj_class {
	COBJ_CLASS_FIELDS;
};

/*
 * Implementation of cobj.
 */
#define COBJ_FIELDS				\
	cobj_ops_t	ops

struct cobj {
	COBJ_FIELDS;
};

/*
 * The ops table is used as a cache of 
 * results from cobj_call_method(3).
 */

#define COBJ_CACHE_SIZE	256

struct cobj_ops {
	cobj_method_t	*cache[COBJ_CACHE_SIZE];
	cobj_class_t	cls;
};

struct cobjop_desc {
	unsigned int	id;		/* unique ID */
	cobj_method_t	deflt;		/* default implementation */
};

/*
 * Shorthand for constructing method tables.
 * 
 * The ternary operator is (ab)used to provoke a warning when FUNC
 * has a signature that is not compatible with cobj method signature.
 */
#define COBJ_METHOD(NAME, FUNC) \
	{ &NAME##_desc, (cobjop_t) (1 ? FUNC : (NAME##_t *)NULL) }

/*
 * Shorthand for finalizing method tables.
 */
#define COBJ_METHOD_END	{ NULL, NULL }

/*
 * Declare a class (which should be defined in another file.
 */
#define DECLARE_CLASS(name) extern struct cobj_class name

/*
 * Define a class with no base classes.
 */
#define DEFINE_CLASS(name, methods, size)     		\
DEFINE_CLASS_0(name, name ## _class, methods, size)

/*
 * Define a class with no base classes. Use like this:
 *
 * DEFINE_CLASS_0(foo, foo_class, foo_methods, sizeof(foo_softc));
 */
#define DEFINE_CLASS_0(name, classvar, methods, size)	\
							\
struct cobj_class classvar = {				\
	#name, methods, size, NULL			\
}

/*
 * Define a class inheriting a single base class. Use like this:
 *
 * DEFINE_CLASS_1(foo, foo_class, foo_methods, sizeof(foo_softc),
 *			  bar);
 */
#define DEFINE_CLASS_1(name, classvar, methods, size,	\
		       base1)				\
							\
static cobj_class_t name ## _baseclasses[] =		\
	{ &base1, NULL };				\
struct cobj_class classvar = {				\
	#name, methods, size, name ## _baseclasses	\
}

/*
 * Define a class inheriting two base classes. Use like this:
 *
 * DEFINE_CLASS_2(foo, foo_class, foo_methods, sizeof(foo_softc),
 *			  bar, baz);
 */
#define DEFINE_CLASS_2(name, classvar, methods, size,	\
	               base1, base2)			\
							\
static cobj_class_t name ## _baseclasses[] =		\
	{ &base1,					\
	  &base2, NULL };				\
struct cobj_class classvar = {				\
	#name, methods, size, name ## _baseclasses	\
}

/*
 * Define a class inheriting three base classes. Use like this:
 *
 * DEFINE_CLASS_3(foo, foo_class, foo_methods, sizeof(foo_softc),
 *			  bar, baz, foobar);
 */
#define DEFINE_CLASS_3(name, classvar, methods, size,	\
		       base1, base2, base3)		\
							\
static cobj_class_t name ## _baseclasses[] =		\
	{ &base1,					\
	  &base2,					\
	  &base3, NULL };				\
struct cobj_class classvar = {				\
	#name, methods, size, name ## _baseclasses	\
}

/*
 * Maintain stats on hits/misses in lookup caches.
 */
#ifdef COBJ_STATS
extern u_int cobj_lookup_hits;
extern u_int cobj_lookup_misses;
#endif

/*
 * Lookup the method in the cache and if 
 * it isn't there look it up the slow way.
 */
#ifdef COBJ_STATS
#define COBJ_CALL_METHOD(OPS,OP) do {				\
	cobjop_desc_t _desc = &OP##_##desc;			\
	cobj_method_t **_cep =					\
	    &OPS->cache[_desc->id & (COBJ_CACHE_SIZE-1)];	\
	cobj_method_t *_ce = *_cep;				\
	if (_ce->desc != _desc) {				\
		_ce = cobj_call_method(OPS->cls,		\
					 _cep, _desc);		\
		cobj_lookup_misses++;				\
	} else							\
		cobj_lookup_hits++;				\
	_m = _ce->func;						\
} while(0)
#else
#define COBJ_CALL_METHOD(OPS,OP) do {				\
	cobjop_desc_t _desc = &OP##_##desc;			\
	cobj_method_t **_cep =					\
	    &OPS->cache[_desc->id & (COBJ_CACHE_SIZE-1)];	\
	cobj_method_t *_ce = *_cep;				\
	if (_ce->desc != _desc)					\
		_ce = cobj_call_method(OPS->cls,		\
					 _cep, _desc);		\
	_m = _ce->func;						\
} while(0)
#endif	/* ! COBJ_STATS */

__BEGIN_DECLS
/*
 * Compile the method table in a class.
 */
int		cobj_class_compile(cobj_class_t cls);

/*
 * Compile the method table, with the caller providing the space for
 * the ops table.(for use before malloc is initialised).
 */
int		cobj_class_compile_static(cobj_class_t cls, cobj_ops_t ops);

/*
 * Free the compiled method table in a class.
 */
int		cobj_class_free(cobj_class_t cls);

/*
 * Allocate memory for and initialise a new object.
 */
cobj_t		cobj_create(cobj_class_t cls);

/*
 * Initialize a pre-allocated object.
 */
int		cobj_init(cobj_t obj, cobj_class_t cls);
int		cobj_init_static(cobj_t obj, cobj_class_t cls);

/*
 * Delete an object. If mtype is non-zero, free the memory.
 */
int		cobj_delete(cobj_t obj);

/*
 * Call method.
 */
cobj_method_t * cobj_call_method(cobj_class_t cls,
				  cobj_method_t **cep,
				  cobjop_desc_t desc);
/*
 * Default method implementation.
 */
int	cobj_nop(void);

__END_DECLS

#define	COBJ_LOCK()
#define	COBJ_UNLOCK()
#define	COBJ_ASSERT(what)

#endif /* !_COBJ_H_ */
