
_AWK= 	/usr/bin/awk

LDADD=  -lcobj

PROG=	foo
SRCS=	main.c foo_if.c foo_if.h

MAN=    

# Build _if.[ch] from _if.m, and clean them when we're done.
__MPATH!=find ${.CURDIR:tA}/ -name \*_if.m
_MFILES=${__MPATH:T:O}
_MPATH=${__MPATH:H:O:u}
.PATH.m: ${_MPATH}
.for _i in ${SRCS:M*_if.[ch]}
_MATCH=M${_i:R:S/$/.m/}
_MATCHES=${_MFILES:${_MATCH}}
.if !empty(_MATCHES)
CLEANFILES+=	${_i}
.endif
.endfor # _i
.m.c:	${.CURDIR}/../tools/makeobjops.awk
	${_AWK} -f ${.CURDIR}/../tools/makeobjops.awk ${.IMPSRC} -c

.m.h:	${.CURDIR}/../tools/makeobjops.awk
	${_AWK} -f ${.CURDIR}/../tools/makeobjops.awk ${.IMPSRC} -h

.include <bsd.prog.mk>
