# Makefile.am - libremotediagproxy.a

APP_COMMON_SOURCES :=
APP_COMMON_INCLUDE :=


common_sources = RemotediagProxy.cpp

common_sources += \
                  utils/Logger.cpp \
                  utils/ProxyIpcClient.cpp \
                  utils/UnixSocketTransport.cpp \
                  utils/IpcMessageHandler.cpp \
                  utils/RemotediagProxyHandler.cpp \
                  services/ApplicationManagerAdapter.cpp \
                  services/CalibManagerAdapter.cpp \
                  services/DiagManagerAdapter.cpp \
                  services/FileStoreAdapter.cpp \
                  services/HttpManagerAdapter.cpp \
                  services/MqttManagerAdapter.cpp \
                  services/SomeipManagerAdapter.cpp \
                  services/LocationManagerAdapter.cpp \
                  services/PPIManagerAdapter.cpp \
                  services/PowerManagerAdapter.cpp \
                  services/VehicleManagerAdapter.cpp \
                  services/RegionManagerAdapter.cpp \
                  services/TimeManagerAdapter.cpp \
                  services/TelephonyManagerAdapter.cpp


app_header_inc = -I$(top_srcdir)
app_header_inc += $(APP_COMMON_INCLUDE)

# Target name DSO
lib_LTLIBRARIES := libremotediagproxy.la

libremotediagproxy_la_CPPFLAGS = \
    $(app_header_inc) \
    -I$(top_srcdir)/.. \
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

libremotediagproxy_la_LDFLAGS = \
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

libremotediagproxy_la_SOURCES := \
    $(common_sources)
