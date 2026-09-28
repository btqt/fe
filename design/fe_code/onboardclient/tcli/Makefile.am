
# Makefile.am - make executable for service layer process

bin_PROGRAMS = tcli_onboardclient

tcli_onboardclient_SOURCES = \
	tclionboardclient.cpp

tcli_onboardclient_CPPFLAGS := \
	-I$(top_srcdir) \
	-Wall \
	-Wno-unused-parameter \
	-D_STDATOMIC_HAVE_ATOMIC \
	$(AM_CPPFLAGS) \
	$(USE_SERVICE_BIN_CFLAGS) \
	$(USE_SECURE_BIN_CFLAGS)


tcli_onboardclient_LDFLAGS = \
	$(USE_DEBUG_LDFLAGS) \
	$(USE_EXTERNAL_LDFLAGS) \
	$(USE_SERVICE_BIN_LDFLAGS) \
	$(USE_SECURE_BIN_LDFLAGS)
