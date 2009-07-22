## Process this file with automake to produce Makefile.in

SUBDIRS = fft2d

bin_PROGRAMS = dct-carver

bindir = $(GIMP_LIBDIR)/plug-ins

dct_carver_SOURCES = \
	dct.c		\
	dct.h		\
	interface.c	\
	interface.h	\
	main.c		\
	main.h		\
	render.c	\
	render.h	

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"
	-g -Wall

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	@LQR_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS) \
		$(LQR_LIBS) \
		fft2d/libfft2d.a
