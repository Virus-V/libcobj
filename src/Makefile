LIB=	cobj

SHLIB_MAJOR=1
SHLIB_MINOR=0

SRCS=	cobj_class.c cobj.c
INCS=	libcobj.h 
NO_MAN= 

LIBADD+= -lpthread -lrt

CFLAGS+= -I${.CURDIR}

.include <bsd.lib.mk>
