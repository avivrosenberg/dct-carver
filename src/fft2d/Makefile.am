## Process this file with automake to produce Makefile.in


noinst_LIBRARIES = libfft2d.a

libfft2d_a_SOURCES = \
	alloc.c \
	alloc.h \
	fftsg.c \
	fftsg2d.c \
	shrtdct.c 

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"			\
	-g -Wall -DUSE_FFT2D_PTHREADS

INCLUDES =\
	-I$(top_srcdir)		\
	-I$(includedir)

