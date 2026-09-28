
# Makefile.am - make executable for service layer process
bin_PROGRAMS = OnboardclientManagerService

OnboardclientManagerService_SOURCES :=\
    services/OnboardclientManagerService_main.cpp \
    services/OnboardclientInputManager.cpp \
    services/OnboardclientManagerService.cpp \
    services/OnboardclientRxHandler.cpp \
    services/OnboardclientTxHandler.cpp \
    services/OnboardclientImpl.cpp \
    tiger/CommunicationManagerAdapter.cpp \
    tiger/SystemPostReceiver.cpp \
    tiger/DiagManagerAdapter.cpp

# @CGA_VARIANT_START{"service/Makefile.am:TIDL_src_additional_CPPFLAGS:__YOUR_CODE__:variant"}
TIDL_src_additional_CPPFLAGS = \
    $(AM_CPPFLAGS)
# @CGA_VARIANT___END{"service/Makefile.am:TIDL_src_additional_CPPFLAGS:__YOUR_CODE__:variant"}

OnboardclientManagerService_CPPFLAGS := \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/service/include \
    -I$(top_srcdir)/service/tiger \
    -Wall \
    -Wno-unused-parameter \
    -fpermissive \
    $(TIDL_src_additional_CPPFLAGS) \
    $(USE_SERVICE_BIN_CFLAGS) \
    $(USE_SECURE_BIN_CFLAGS)


OnboardclientManagerService_LDFLAGS = \
    -liapplication \
    -lionboardclient \
    -licommunication \
    -lidiag \
    -lpthread \
    $(USE_SERVICE_BIN_LDFLAGS) \
    $(USE_DEBUG_LDFLAGS) \
    $(USE_EXTERNAL_LDFLAGS) \
    $(USE_SECURE_BIN_LDFLAGS)


# @CGA_VARIANT_START{"service/Makefile.am:TIDL_custom_area:__YOUR_CODE__:variant"}
# @CGA_VARIANT___END{"service/Makefile.am:TIDL_custom_area:__YOUR_CODE__:variant"}
