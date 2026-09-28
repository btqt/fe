
# Makefile.am - libionboardclient.a

# @CGA_VARIANT_START{"interface/Makefile.am:TIDL_src_additional_SOURCES:__YOUR_CODE__:variant"}
TIDL_src_additional_SOURCES = \
    $(top_srcdir)/IOnboardclientManagerReceiver.cpp \
    $(top_srcdir)/IOnboardclientManagerService.cpp
# @CGA_VARIANT___END{"interface/Makefile.am:TIDL_src_additional_SOURCES:__YOUR_CODE__:variant"}

libionboardclient_sources:= \
    $(top_srcdir)/OBCCanInfo.cpp \
    $(top_srcdir)/OBCResponseEventInfo.cpp \
    $(top_srcdir)/OBCTransportInfo.cpp \
    $(top_srcdir)/OBCUDSResInfo.cpp \
    $(top_srcdir)/OBCConnectInfo.cpp \
    $(top_srcdir)/IOnboardclientManagerReceiver.cpp \
    $(top_srcdir)/IOnboardclientManagerService.cpp \
    $(top_srcdir)/OnboardclientManager.cpp

# Target name
lib_LIBRARIES := libionboardclient.a

libionboardclient_includedir = $(includedir)/services/OnboardclientManagerService
libionboardclient_include_HEADERS = $(srcdir)/include/services/OnboardclientManagerService/*

libionboardclient_a_SOURCES := \
    $(libionboardclient_sources)

# @CGA_VARIANT_START{"interface/Makefile.am:TIDL_src_additional_CPPFLAGS:__YOUR_CODE__:variant"}
TIDL_src_additional_CPPFLAGS = \
    $(AM_CPPFLAGS)
# @CGA_VARIANT___END{"interface/Makefile.am:TIDL_src_additional_CPPFLAGS:__YOUR_CODE__:variant"}

libionboardclient_a_CPPFLAGS := \
    -I$(top_srcdir)/include \
    -I./include \
    -Wall \
    -Wno-unused-parameter \
    $(TIDL_src_additional_CPPFLAGS) \
    $(USE_INTERFACE_LIB_CFLAGS) \
    $(USE_SECURE_LIB_CFLAGS)


libionboardclient_a_LDFLAGS := \
    $(USE_DEBUG_LDFLAGS) \
    $(USE_INTERFACE_LIB_LDFLAGS) \
    $(USE_SECURE_LIB_LDFLAGS)


#Build a DSO too
lib_LTLIBRARIES := libionboardclient.la
libionboardclient_la_SOURCES := $(libionboardclient_a_SOURCES)
libionboardclient_la_CPPFLAGS := $(libionboardclient_a_CPPFLAGS)
libionboardclient_la_LDFLAGS := $(libionboardclient_a_LDFLAGS)

# @CGA_VARIANT_START{"interface/Makefile.am:TIDL_custom_area:__YOUR_CODE__:variant"}
# @CGA_VARIANT___END{"interface/Makefile.am:TIDL_custom_area:__YOUR_CODE__:variant"}
