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

common_sources += \
        services/ApplicationManagerAdapter.cpp \
        services/CalibManagerAdapter.cpp \
        services/DiagManagerAdapter.cpp \
        services/HttpManagerAdapter.cpp \
        services/LocationManagerAdapter.cpp \
        services/MqttManagerAdapter.cpp \
        services/OnboardclientManagerAdapter.cpp \
        services/PowerManagerAdapter.cpp \
        services/PPIManagerAdapter.cpp \
        services/VehicleManagerAdapter.cpp \
        services/RegionManagerAdapter.cpp \
        utils/RemotediagHandler.cpp \
        utils/Logger.cpp \
        utils/RemoteDiagDatastore.cpp \
        sldd/RemoteDiagSLDD.cpp \
        diagprocess/DirectCommand/RemoteDirectCommand.cpp \
        diagprocess/DiagTrigger.cpp \
        diagprocess/TriggerIDGenerator.cpp \
        diagprocess/CenterReqData.cpp \
        diagprocess/CRCManager.cpp \
        diagprocess/RoBSSR/RemoteRoBSSR.cpp

# Target name DSO
lib_LTLIBRARIES := libremotediag.la

libremotediag_la_CPPFLAGS = \
    $(app_header_inc) \
    -I$(top_srcdir)/diagprocess/ \
    -I$(top_srcdir)/diagprocess/RoB/ \
    -I$(top_srcdir)/diagprocess/UDS/ \
    -I$(top_srcdir)/diagprocess/DirectCommand/ \
    -I$(top_srcdir)/services/ \
    -I$(SDKTARGETSYSROOT)/usr/include/servicelayer \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/mqtt \
    -I$(stincdir)/servicelayer \
    -I${stincdir}/glib-2.0 \
    -I$(stincdir)/../lib/glib-2.0/include \
    -D_MORE_MARSHM \
    -O2 \
    -g \
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
    -liecall \
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
    -export-symbols-regex 'createApplication'

requiredlibs := -lpthread

libremotediag_la_SOURCES := \
    $(common_sources)
