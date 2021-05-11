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
#include <sys/queue.h>

#include <stdlib.h>

#include <libcobj.h>

/*
 * Example for a simplefied queue(3).
 */

#include "tailq_if.h"

/*
 * Forward declarations.
 */
typedef struct tailq_item	*tailq_item_t;
typedef struct tailq_obj	*tailq_obj_t;

/*
 * Element holds reference.
 */
struct tailq_item {
	TAILQ_ENTRY(tailq_item) ti_next;
	void		*ti_data;
};

/*
 * Software-context for an instance of tailq_class(3).
 */
struct tailq_obj {
	COBJ_FIELDS;
	TAILQ_HEAD(, tailq_item) to_cache;
};

/*
 * Enqueue.
 */
static int
tailq_add(cobj_t o, void *arg)
{
	tailq_obj_t to;
	tailq_item_t ti;
	
	if (arg == NULL)
		return (-1);
	
	if ((to = (tailq_obj_t)o) == NULL)
		return (-1);
	
	if ((ti = calloc(1, sizeof(struct tailq_item))) == NULL)
		return (-1);

	ti->ti_data = arg;
	
	TAILQ_INSERT_TAIL(&to->to_cache, ti, ti_next);
	
	return (0);
}

/*
 * Dequeue.
 */
static void *
tailq_poll(cobj_t o)
{
	tailq_obj_t to;
	tailq_item_t ti;
	void *data;
	
	if ((to = (tailq_obj_t)o) == NULL)
		return (NULL);
	
	if ((ti = TAILQ_FIRST(&to->to_cache)) == NULL) 
		return (NULL);
	
	TAILQ_REMOVE(&to->to_cache, ti, ti_next);	
	
	data = ti->ti_data;
	ti->ti_data = NULL;
	
	free(ti);
	
	return (data);
}

/*
 * Flush queue.
 */
static int
tailq_flush(cobj_t o)
{
	tailq_obj_t to;
	tailq_item_t ti;
	
	if ((to = (tailq_obj_t)o) == NULL)
		return (-1);
	
	while (!TAILQ_EMPTY(&to->to_cache)) {
		ti = TAILQ_FIRST(&to->to_cache);
		TAILQ_REMOVE(&to->to_cache, ti, ti_next);	
		ti->ti_data = NULL;
		free(ti);
	}
	
	return (0);	
}

/*
 * Ctor.
 */
static cobj_t
tailq_create(cobj_class_t c)
{
	tailq_obj_t to;
	
	if ((to = (tailq_obj_t)cobj_create(c)) == NULL)
		return (NULL);
	
	TAILQ_INIT(&to->to_cache);
	
	return ((cobj_t)to);
}

/*
 * Dtor.
 */
static int
tailq_destroy(cobj_class_t c, cobj_t o)
{

	if (tailq_flush(o) != 0)
		return (-1);
		
	return (cobj_delete(o));
}

static cobj_method_t tailq_methods[] = {
	/* public methods */
	COBJ_METHOD(tailq_add,		tailq_add),
	COBJ_METHOD(tailq_poll,		tailq_poll),
	COBJ_METHOD(tailq_flush,		tailq_flush),
	
	/* static methods */
	COBJ_METHOD(tailq_create,		tailq_create),
	COBJ_METHOD(tailq_destroy,		tailq_destroy),
	COBJ_METHOD_END
};

DEFINE_CLASS(tailq, tailq_methods, sizeof(struct tailq_obj));
