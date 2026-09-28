# Makefile.am - libremotediag.a

APP_COMMON_SOURCES :=
APP_COMMON_INCLUDE :=

include utils/utils.am
include diagprocess/SSR/SSR.am
include diagprocess/DTC/DTC.am
include diagprocess/OTA/OTA.am
include diagprocess/UDS/UDS.am
include diagprocess/RoB/RoB.am
include diagprocess/Warning/Warning.am
include diagprocess/EcuInformation/EcuInformation.am
include diagprocess/LastUpload/LastUpload.am
include diagprocess/RoBMonitoring/RoBMonitoring.am
include diagprocess/RoBOccurrence/RoBOccurrence.am


common_sources = Remotediag.cpp
common_sources += $(APP_COMMON_SOURCES)

app_header_inc = -I$(top_srcdir)
app_header_inc += $(APP_COMMON_INCLUDE)

# Adapter implementations: services/ carries the LXC (partition separation)
# code and is the default; non-LXC builds use the original adapters kept at
# services_org/services/ (nested so quoted "services/X.h" resolves via -I).
if ENABLE_LGE_LXC
adapter_dir = services
adapter_inc = -I$(top_srcdir)/services/
else
adapter_dir = services_org/services
adapter_inc = -I$(top_srcdir)/services_org -I$(top_srcdir)/services_org/services
endif

common_sources += \
        $(adapter_dir)/ApplicationManagerAdapter.cpp \
        $(adapter_dir)/CalibManagerAdapter.cpp \
        $(adapter_dir)/DiagManagerAdapter.cpp \
        $(adapter_dir)/HttpManagerAdapter.cpp \
        $(adapter_dir)/LocationManagerAdapter.cpp \
        $(adapter_dir)/MqttManagerAdapter.cpp \
        $(adapter_dir)/OnboardclientManagerAdapter.cpp \
        $(adapter_dir)/PowerManagerAdapter.cpp \
        $(adapter_dir)/PPIManagerAdapter.cpp \
        $(adapter_dir)/VehicleManagerAdapter.cpp \
        $(adapter_dir)/RegionManagerAdapter.cpp \
        $(adapter_dir)/SomeipManagerAdapter.cpp \
        utils/RemotediagHandler.cpp \
        utils/Logger.cpp \
        sldd/RemoteDiagSLDD.cpp \
        diagprocess/DirectCommand/RemoteDirectCommand.cpp \
        diagprocess/DiagTrigger.cpp \
        diagprocess/TriggerIDGenerator.cpp \
        diagprocess/CenterReqData.cpp \
        diagprocess/CRCManager.cpp \
        diagprocess/RoBSSR/RemoteRoBSSR.cpp

if ENABLE_LGE_LXC
common_sources += \
        remotediagproxy/utils/UnixSocketTransport.cpp \
        remotediagproxy/utils/IpcMessageHandler.cpp
endif

# Target name DSO
lib_LTLIBRARIES := libremotediag.la

# Automake's default '-I. -I$(srcdir)' would resolve "services/X.h" before
# $(adapter_inc); keep only -I. (config.h) — -I$(top_srcdir) follows below.
DEFAULT_INCLUDES = -I.

libremotediag_la_CPPFLAGS = \
    $(adapter_inc) \
    $(app_header_inc) \
    -I$(top_srcdir)/diagprocess/ \
    -I$(top_srcdir)/diagprocess/RoB/ \
    -I$(top_srcdir)/diagprocess/UDS/ \
    -I$(top_srcdir)/diagprocess/DirectCommand/ \
    -I$(SDKTARGETSYSROOT)/usr/include/servicelayer \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/mqtt \
    -I$(stincdir)/servicelayer \
    -I${stincdir}/glib-2.0 \
    -I$(stincdir)/../lib/glib-2.0/include \
    -D_MORE_MARSHM \
    -O2 \
    -g \
    -std=c++14 \
    -Wall \
    -Wno-unused-parameter \
    -DHAVE_SYS_UIO_H \
    -DREMOTEDIAG_SLDD_TEST \
    -rdynamic \
    -fpermissive \
    -fno-omit-frame-pointer \
    $(AM_CPPFLAGS)

libremotediag_la_LDFLAGS = \
    -lpthread \
    -lapplication \
    -liapplication \
    -licommunication \
    -lihttp \
    -lprotohttp \
    -lprotobuf \
	-licalib \
	-lidiag \
    -luuid \
    -liaudio \
    -litime \
    -lipower \
    -lilocation \
    -liemljson \
    -lcorebase \
    -lwatchdog_client \
    -litelephony \
    -shared \
    -module \
    -lutils \
    -ldlt \
    -lbinder \
    -liregion \
    -lserviceutils \
    -lihmi \
    -lm \
    -ldl \
    -lpcre \
    -lcontext \
    -lproperty \
    -lioperationmode \
    -lialarm \
    -export-dynamic \
    -lisomeipprovider \
    -lisomeipconsumer \
    -lidcemqttproxy \
    -lippi \
    -lionboardclient \
    -lilgefileio \
    -lutils \
    -lsqlite3 \
    -ltdm_frontend \
    -lcrypto \
    -export-symbols-regex 'createApplication'

requiredlibs := -lpthread

libremotediag_la_SOURCES := \
    $(common_sources)
