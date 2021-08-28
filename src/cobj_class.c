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

static int cobj_next_id = 1;

/*
 * This method structure is used to initialise new caches. Since the
 * desc pointer is NULL, it is guaranteed never to match any read
 * descriptors.
 */
static const struct cobj_method null_method = {
    NULL,
    NULL,
};

/*
 * Initialize a class.
 */

static void
cobj_class_compile_common(cobj_class_t cls, cobj_ops_t ops) {
  cobj_method_t *m;
  int i;

  /*
	 * Don't do anything if we are already compiled.
	 */
  if (cls->ops != NULL)
    return;

  /*
	 * First register any methods which need it.
	 */
  for (m = cls->methods; m->desc; m++) {
    if (m->desc->id == 0)
      m->desc->id = cobj_next_id++;
  }

  /*
	 * Then initialise the ops table.
	 */
  for (i = 0; i < COBJ_CACHE_SIZE; i++)
    ops->cache[i] = &null_method;

  ops->cls = cls;
  cls->ops = ops;
}

int cobj_class_compile(cobj_class_t cls) {
  cobj_ops_t ops;

  COBJ_ASSERT(MA_NOTOWNED);

  if (cls == NULL)
    return (-1);

  /*
	 * Allocate space for the compiled ops table.
	 */
  if ((ops = calloc(1, sizeof(struct cobj_ops))) == NULL)
    return (-1);

  sem_wait(&cobj_lock);

  /*
	 * We may have lost a race for cobj_class_compile here - check
	 * to make sure someone else hasn't already compiled this
	 * class.
	 */
  if (cls->ops != NULL) {
    sem_post(&cobj_lock);
    free(ops);
  } else {
    cobj_class_compile_common(cls, ops);
    sem_post(&cobj_lock);
  }

  return (0);
}

int cobj_class_compile_static(cobj_class_t cls, cobj_ops_t ops) {

  if (ops == NULL)
    return (-1);

  if (cls == NULL)
    return (-1);
  /*
	 * Increment refs to make sure that
	 * the ops table is not freed.
	 */

  cls->refs++;
  cobj_class_compile_common(cls, ops);

  return (0);
}

/*
 * Release bound methods.
 */
int cobj_class_free(cobj_class_t cls) {
  void *ops = NULL;

  COBJ_ASSERT(MA_NOTOWNED);

  if (cls == NULL)
    return (-1);

  sem_wait(&cobj_lock);

  /*
	 * Protect against a race between cobj_create and cobj_delete.
	 */
  if (cls->refs == 0) {
    /*
		 * For now we don't do anything to unregister any methods
		 * which are no longer used.
		 */

    /*
		 * Free memory and clean up.
		 */
    ops = cls->ops;
    cls->ops = NULL;
  }

  sem_post(&cobj_lock);

  if (ops != NULL)
    free(ops);

  return (0);
}

/*
 * Call a specific method.
 */

static cobj_method_t *
cobj_call_method_at_class(cobj_class_t cls, cobjop_desc_t desc) {
  cobj_method_t *methods;
  cobj_method_t *ce;

  methods = cls->methods;

  for (ce = methods; ce && ce->desc; ce++) {
    if (ce->desc == desc) {
      return (ce);
    }
  }

  return (NULL);
}

static cobj_method_t *
cobj_call_method_at_mi(cobj_class_t cls, cobjop_desc_t desc) {
  cobj_method_t *ce;
  cobj_class_t *basep;

  if ((ce = cobj_call_method_at_class(cls, desc)) != NULL)
    return (ce);

  if ((basep = cls->baseclasses) != NULL) {
    for (; *basep; basep++) {
      ce = cobj_call_method_at_mi(*basep, desc);
      if (ce != NULL)
        return (ce);
    }
  }

  return (NULL);
}

cobj_method_t *
cobj_call_method(cobj_class_t cls,
                 cobj_method_t **cep,
                 cobjop_desc_t desc) {
  cobj_method_t *ce;

  if ((ce = cobj_call_method_at_mi(cls, desc)) == NULL)
    ce = &desc->deflt;

  if (cep != NULL)
    *cep = ce;

  return (ce);
}

int cobj_nop(void) {

  return (-1);
}
