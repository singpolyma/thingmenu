# thingmenu metadata
NAME = thingmenu
VERSION = 0.7

# Customize below to fit your system

# paths
PREFIX ?= /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -L${X11LIB} -lc -lX11

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
CFLAGS = -g -std=c99 -pedantic -Wall ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS}
#LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc

