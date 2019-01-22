/*
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <sys/file.h>

#include <assert.h>
#include <db.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "dom_var.h"

#define _PATH_DOMCONF 	"dom.conf" 	/* XXX: /etc/dom.conf */

/*
 * Record on Berkeley db(3) for named DOM node map. 
 */
struct dom_typemap {
	char 	*dtm_name; 
	char 	*dtm_type;
};

static char dom_buf[LINE_MAX];
static struct dom_type *dom_typevec = NULL;
static DB *dom_db = NULL;

/*
 * Register module implements DOM type.
 */
void 
dom_type_register(struct dom_type *dt)
{
	dt->dt_next = dom_typevec;
	dom_typevec = dt;
}

/*
 * Fetch DOM type maps to node.
 */
static struct dom_typemap *
dom_type_get_internal(const char *tok)
{
	DBT key, data;
	struct dom_typemap *dtm;

	key.size = strlen(tok);
	
	if (key.size >= sizeof(dom_buf) - 1)
		return (NULL);
	
	(void)memcpy(dom_buf, tok, key.size);
	key.data = dom_buf;

	switch ((*dom_db->get)(dom_db, &key, &data, 0)) {
	case RET_SUCCESS:
		(void)memmove(&dtm, data.data, sizeof(dtm));
		(void)printf("( k, v ) := ( %s , %s )\n", 
			dtm->dtm_name, dtm->dtm_type);
				
		break;
	case RET_ERROR: 	/* FALLTHROUGH */
	case RET_SPECIAL:		
		dtm = NULL;
		break;
	default:
		errx(EX_UNAVAILABLE, "%s: db(3) get", __func__);
		break; 	/* NOTREACHED */
	}
	return (dtm);
}

struct dom_type *
dom_type_get(const char *key)
{
	struct dom_typemap *dtm;
	
	if (key == NULL)
		return (NULL);
	
	if ((dtm = dom_type_get_internal(key)) == NULL)
		return (NULL);
	
	return (dom_type_get_by_name(dtm->dtm_type));
}

struct dom_type *
dom_type_get_by_id(uint32_t cookie)
{
	struct dom_type *dt;
	
	for (dt = dom_typevec; dt != NULL; dt = dt->dt_next) {
		if (dt->dt_cookie == cookie)
			return (dt);	
	}
	return (NULL);
}

struct dom_type *
dom_type_get_by_name(const char *name)
{
	struct dom_type *dt;
	
	for (dt = dom_typevec; dt != NULL; dt = dt->dt_next) {
		if (strcmp(dt->dt_type, name) == 0)
			return (dt);	
	}
	return (NULL);
}

/*
 * Initialize db(3).
 */

static int 
dom_fgetln(FILE *fp)
{	
	struct dom_typemap *dtm;
	char *p, *tok;
	size_t len;	
	DBT key, data;

next_line:	
	if ((p = fgetln(fp, &len)) == NULL) 
		return (0);	
	
	if (len > 0 && p[len - 1] == '\n')
		len--;
	
	if (len >= sizeof(dom_buf) - 1) 
		errx(EX_DATAERR, "%s: buf too small", __func__);

	(void)memcpy(dom_buf, p, len);
	
	dom_buf[len] = '\0';

	/* Ignore comments: ^[ \t]*# */	
	for (p = dom_buf; *p != '\0'; p++) {
		if (*p != ' ' && *p != '\t')
			break;
	}
	
	if (*p == '#' || *p == '\0') 
		goto next_line;	
	
	/* Strip off [ascending] comments. */ 		
	if ((tok = strsep(&p, "#")) != NULL) {
		/* Truncate and extract token. */
		if ((p = strsep(&tok, " ")) != NULL) {
			len = strlen(p);
			(void)memcpy(dom_buf, p, len);
		} else {
			len = strlen(tok);
			(void)memcpy(dom_buf, tok, len);
		}
		dom_buf[len] = '\0';
	}	
	p = dom_buf;
			
	if ((dtm = calloc(1, sizeof(*dtm))) == NULL)
		errx(EX_OSERR, "%s: calloc(3)", __func__);

	/* Extract key. */		
	if ((tok = strsep(&p, ":")) == NULL) 
		errx(EX_DATAERR, "%s: strsep(3)", __func__);
		
	if ((dtm->dtm_name = strdup(tok)) == NULL)
		errx(EX_OSERR, "%s: strdup(3)", __func__);
	
	/* Extract dom_type. */			
	if ((tok = strsep(&p, ":")) == NULL) 
		errx(EX_DATAERR, "%s: strsep(3)", __func__);
		
	if ((dtm->dtm_type = strdup(tok)) == NULL)
		errx(EX_OSERR, "%s: strdup(3)", __func__);

	/* Put record on in-memory db(3). */		
	key.data = dtm->dtm_name;
	key.size = strlen(dtm->dtm_name);
	
	data.size = sizeof(struct dom_typreq *);
	data.data = &dtm;

	switch ((*dom_db->put)(dom_db, &key, &data, R_NOOVERWRITE)) {
	case RET_SUCCESS:
		break;
	default:	
		errx(EX_DATAERR, "%s: db(3) put", __func__);
		break; /* NOTREACHED */
	}	
	return (1);
}

static  __attribute__((constructor)) void  
dom_init(void)
{
	FILE *fp;
	
	if ((fp = fopen(_PATH_DOMCONF, "r")) == NULL)
		errx(EX_NOINPUT, "%s: fopen(3) %s", __func__, _PATH_DOMCONF);

	if (flock(fileno(fp), LOCK_EX) < 0)
		errx(EX_OSERR, "%s: flock(2)", __func__);

	if ((dom_db = dbopen(NULL, O_RDWR, 0, DB_BTREE, NULL)) == NULL)
		errx(EX_OSERR, "%s: dbopen(3)", __func__);	

	while (dom_fgetln(fp))
		continue;

	if (fclose(fp) < 0)
		errx(EX_NOINPUT, "%s: fclose(3)", __func__);
}

/*
 * Flush by db(3) cached data.
 */
static  __attribute__((destructor)) void
dom_fini(void)
{
	(void)(*dom_db->close)(dom_db);
}
