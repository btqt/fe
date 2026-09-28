# Makefile.am - make executable for service layer process

bin_PROGRAMS = sldd_onboardclient

sldd_onboardclient_SOURCES = \
    frame/man.cpp \
    frame/sldd.cpp \
    SLDD_onboardclient-service.cpp

sldd_onboardclient_CPPFLAGS := \
    -I$(top_srcdir) \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/sldd \
    -I$(top_srcdir)/sldd/frame \
    -Wall \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -fpermissive \
    -Wno-format-security \
    -D_STDATOMIC_HAVE_ATOMIC \
    $(AM_CPPFLAGS) \
    $(USE_SERVICE_BIN_CFLAGS) \
    $(USE_SECURE_BIN_CFLAGS)

sldd_onboardclient_LDFLAGS = \
    -lionboardclient  \
    -lpthread \
    -lrt \
    -lutils \
    $(USE_DEBUG_LDFLAGS) \
    $(USE_EXTERNAL_LDFLAGS) \
    $(USE_SERVICE_BIN_LDFLAGS) \
    $(USE_SECURE_BIN_LDFLAGS)
