# thinglaunch
# See LICENSE file for copyright and license details.

include config.mk

SRC = ${NAME}.c

OBJ = ${SRC:.c=.o}

all: options ${NAME}

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

${NAME}: config.h ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f ${NAME} ${OBJ} ${NAME}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${NAME}-${VERSION}
	@cp -R LICENSE Makefile README.md config.mk \
		${SRC} *.h ${NAME}-${VERSION}
	@tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	@gzip ${NAME}-${VERSION}.tar
	@rm -rf ${NAME}-${VERSION}

etc:
	@echo installing etc files into ${DESTDIR}/etc/${NAME}
	@mkdir -p ${DESTDIR}/etc/${NAME}
	@cp -R etc/${NAME}/* ${DESTDIR}/etc/${NAME}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f ${NAME} ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/${NAME}

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/${NAME}

.PHONY: all options clean dist install uninstall
