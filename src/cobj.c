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
 */
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
 
#include <sys/cdefs.h>
#include <sys/types.h>

#include <stdlib.h>

#include <libcobj.h>

#ifdef COBJ_STATS
u_int kobj_lookup_hits = 0;
u_int kobj_lookup_misses = 0;
#endif

/*
 * Allocate and initialize the new object.
 */
 
static void
cobj_init_common(cobj_t obj, cobj_class_t cls)
{

	obj->ops = cls->ops;
	cls->refs++;
} 

int
cobj_init_static(cobj_t obj, cobj_class_t cls)
{
	if (cls == NULL)
		return (-1);
		
	if (obj == NULL)
		return (-1);
	
	cobj_init_common(obj, cls);
	
	return (0);
}

int
cobj_init(cobj_t obj, cobj_class_t cls)
{
	
	COBJ_ASSERT(MA_NOTOWNED);
	
	if (cls == NULL)
		return (-1);
		
	if (obj == NULL)
		return (-1);
retry:
	COBJ_LOCK();

	/*
	 * Consider compiling the class' method table.
	 */
	if (cls->ops == NULL) {
		/*
		 * cobj_class_compile doesn't want the lock held
		 * because of the call to calloc(3) - we drop the lock
		 * and re-try.
		 */
		COBJ_UNLOCK();
		
		if (cobj_class_compile(cls) != 0)	
			return (-1);
		
		goto retry;
	}

	cobj_init_common(obj, cls);

	COBJ_UNLOCK();
	
	return (0);
}

cobj_t
cobj_create(cobj_class_t cls)
{
	cobj_t obj;

	if (cls == NULL)
		return (NULL);

	if (cls->size < sizeof(struct cobj))
		return (NULL);

	if ((obj = calloc(1, cls->size)) == NULL)
		return (NULL);
	
	if (cobj_init(obj, cls) != 0) {
		free(obj);
		return (NULL);
	}
	
	return (obj);
}

/*
 * Destroy an object.
 */
int
cobj_delete(cobj_t obj)
{
	cobj_class_t cls;
	int refs;

	if (obj == NULL)
		return (-1);

	cls = obj->ops->cls;

	/*
	 * Consider freeing the compiled method table for the class
	 * after its last instance is deleted. As an optimisation, we
	 * should defer this for a short while to avoid thrashing.
	 */
	COBJ_ASSERT(MA_NOTOWNED);
	COBJ_LOCK();
	cls->refs--;
	refs = cls->refs;
	COBJ_UNLOCK();

	if (refs == 0)
		cobj_class_free(cls);

	obj->ops = NULL;
	free(obj);

	return (0);
}
