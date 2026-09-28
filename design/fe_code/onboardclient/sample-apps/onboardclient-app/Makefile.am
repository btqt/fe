
# Makefile.am - libonboardclient.a

common_sources:= \
	onboardclientApplication.cpp

# Target name DSO
lib_LTLIBRARIES := libonboardclient.la

libonboardclient_la_CPPFLAGS := \
	-I$(SDKTARGETSYSROOT)/usr/include/servicelayer \
	-I$(top_srcdir)/include \
	-I$(stincdir)/services \
	-D_MORE_MARSHM \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	-I$(stincdir)/servicelayer \
	-O2 \
	-g \
	-Wall \
	-Wno-unused-parameter \
	-DHAVE_SYS_UIO_H \
	-rdynamic \
	-fpermissive


libonboardclient_la_LDFLAGS = \
	-lpthread \
	-liapplication \
	-lionboardclient \
	-shared \
	-module \
	-export-dynamic \
	-export-symbols-regex 'createApplication'


requiredlibs := -lpthread

libonboardclient_la_SOURCES := \
	$(common_sources)
