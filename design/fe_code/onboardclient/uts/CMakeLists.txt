


# this is CMakeLists.txt for tiger_desktop
MESSAGE ( STATUS " ___ TIGER_CI_RELEASE(legacy) ___" )

cmake_minimum_required(VERSION 2.8)


# ____TODO____--------- It depends on project build Env.


# set sysroot
SET( CMAKE_SYSROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../../apps_proc/oe-core/build/tmp-glibc/sysroots/mdm9607 )

## specify the cross compiler
SET( CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/../x86_64-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc )
SET( CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/../x86_64-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-g++ )
SET( QEMU_BIN_PATH      ${CMAKE_SYSROOT}/../x86_64-linux/usr/bin/qemu-arm )
SET(MACHINE_QEMU_ARM 1)

MESSAGE("Print SET ENV <===================================>")
MESSAGE( "CMAKE_SYSTROOT : ${CMAKE_SYSROOT})
MESSAGE( "CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE( "CMAKE_CXX_COMPILER : ${CMAKE_CXX_COMPILER}" )
MESSAGE( "QEMU_BIN_PATH      : ${QEMU_BIN_PATH}" )

    # set defines
    ADD_DEFINITIONS (
    	-DHAVE_CONFIG_H
    	-D_DEVICE_NAD=1
    	-D_DEBUG=1
    	-D_JSONC=1
    	-D__USE_DEBUG__
    	-D_FORTIFY_SOURCE=2
    	-D__UNITTEST__
        -DAPP_FEATURE
    )

    SET ( UTS_ARCH_DEP_COMPILE_OPTIONS
	        -march=armv7-a
	        -mfloat-abi=softfp
	        -mfpu=neon
            -include arch/linux-arm/OEConfig.h
    )
    MESSAGE( STATUS " ${UTS_ARCH_DEP_COMPILE_OPTIONS} ")


# ____END_TODO____


# set include paths
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../../service
	${CMAKE_CURRENT_SOURCE_DIR}/../../interface
	${CMAKE_CURRENT_SOURCE_DIR}/../../interface/include
	${CMAKE_SYSROOT}/usr/include
	${CMAKE_SYSROOT}/usr/include/utils
	${CMAKE_SYSROOT}/usr/include/servicelayer
)

# set c cxx flags
ADD_COMPILE_OPTIONS (
	-Werror-implicit-function-declaration
	--sysroot=${CMAKE_SYSROOT}
    ${UTS_ARCH_DEP_COMPILE_OPTIONS}
	-Wall
	-Wno-unused-parameter
	-std=c++11
	-funwind-tables
	-Werror=return-type
	-g
	-O0
	-fexpensive-optimizations
	-frename-registers
	-fomit-frame-pointer
	-g1
	-fvisibility-inlines-hidden
	-fpermissive
	-fstack-protector-all
	-pie
	-fPIE
	-fprofile-arcs
	-ftest-coverage
	-fno-exceptions
	-fno-inline
	-fno-omit-frame-pointer
	-fno-optimize-sibling-calls
	-g
	-fpermissive
	-Wno-write-strings
	-Wno-return-type
	-fno-access-control
	-fno-delete-null-pointer-checks
)


# set link directory
link_directories(
	${CMAKE_SYSROOT}/usr/lib
	${CMAKE_SYSROOT}/lib
)


# set target properties
SET(
	CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# set target list
SET(
	ALL_TARGET 
	OnboardclientManager
	IOnboardclientManagerService
	OnboardclientInputManager
	OnboardclientManagerService
	IOnboardclientManagerReceiver
	OnboardclientManagerService_main
	OBCCanInfo
	OBCResponseEventInfo
	OBCTransportInfo
	OBCUDSResInfo
	OBCErrCode
	OBCPriorityType
	OBCProtocolType
	OBCUdsResponseType
)


# set coverage module
SET(
	CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

include(CodeCoverage)

set(COVERAGE_EXCLUDES '/usr/*' '*unittests/testscripts*' '*.h' '*4.9.2/*')
SETUP_TARGET_FOR_COVERAGE(
	NAME all_coverage
	EXECUTABLE ${ALL_TARGET}
	DEPENDENCIES ${ALL_TARGET}
	)
SETUP_TARGET_FOR_UTS_RUNNER(
	NAME uts_run
	EXECUTABLE ${ALL_TARGET}
	DEPENDENCIES ${ALL_TARGET}
	)

#=================================================================================================

# set filelist for each target
SET ( OnboardclientManager_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OnboardclientManager.cpp
)

add_executable ( OnboardclientManager ${OnboardclientManager_SRC_FILES} )

# set library for each target
target_link_libraries( OnboardclientManager
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME OnboardclientManager_coverage
	EXECUTABLE OnboardclientManager
	DEPENDENCIES OnboardclientManager
	)

#=================================================================================================

SET ( IOnboardclientManagerService_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_IOnboardclientManagerService.cpp
)

add_executable ( IOnboardclientManagerService ${IOnboardclientManagerService_SRC_FILES} )

# set library for each target
target_link_libraries( IOnboardclientManagerService
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME IOnboardclientManagerService_coverage
	EXECUTABLE IOnboardclientManagerService
	DEPENDENCIES IOnboardclientManagerService
	)

#=================================================================================================

SET ( OnboardclientInputManager_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OnboardclientInputManager.cpp
)

add_executable ( OnboardclientInputManager ${OnboardclientInputManager_SRC_FILES} )

# set library for each target
target_link_libraries( OnboardclientInputManager
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME OnboardclientInputManager_coverage
	EXECUTABLE OnboardclientInputManager
	DEPENDENCIES OnboardclientInputManager
	)

#=================================================================================================

SET ( OnboardclientManagerService_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OnboardclientManagerService.cpp
)

add_executable ( OnboardclientManagerService ${OnboardclientManagerService_SRC_FILES} )

# set library for each target
target_link_libraries( OnboardclientManagerService
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME OnboardclientManagerService_coverage
	EXECUTABLE OnboardclientManagerService
	DEPENDENCIES OnboardclientManagerService
	)

SET ( IOnboardclientManagerReceiver_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_IOnboardclientManagerReceiver.cpp
)

add_executable ( IOnboardclientManagerReceiver ${IOnboardclientManagerReceiver_SRC_FILES} )

# set library for each target
target_link_libraries( IOnboardclientManagerReceiver
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME IOnboardclientManagerReceiver_coverage
	EXECUTABLE IOnboardclientManagerReceiver
	DEPENDENCIES IOnboardclientManagerReceiver
	)

#=================================================================================================

SET ( OnboardclientManagerService_main_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OnboardclientManagerService_main.cpp
)

add_executable ( OnboardclientManagerService_main ${OnboardclientManagerService_main_SRC_FILES} )

# set library for each target
target_link_libraries( OnboardclientManagerService_main
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME OnboardclientManagerService_main_coverage
	EXECUTABLE OnboardclientManagerService_main
	DEPENDENCIES OnboardclientManagerService_main
	)

#=================================================================================================

SET ( OBCCanInfo_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCCanInfo.cpp
)

add_executable ( OBCCanInfo ${OBCCanInfo_SRC_FILES} )

# set library for each target
target_link_libraries( OBCCanInfo
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCResponseEventInfo_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCResponseEventInfo.cpp
)

add_executable ( OBCResponseEventInfo ${OBCResponseEventInfo_SRC_FILES} )

# set library for each target
target_link_libraries( OBCResponseEventInfo
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCTransportInfo_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCTransportInfo.cpp
)

add_executable ( OBCTransportInfo ${OBCTransportInfo_SRC_FILES} )

# set library for each target
target_link_libraries( OBCTransportInfo
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCUDSResInfo_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCUDSResInfo.cpp
)

add_executable ( OBCUDSResInfo ${OBCUDSResInfo_SRC_FILES} )

# set library for each target
target_link_libraries( OBCUDSResInfo
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)


#=================================================================================================


SET ( OBCErrCode_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCErrCode.cpp
)

add_executable ( OBCErrCode ${OBCErrCode_SRC_FILES} )

# set library for each target
target_link_libraries( OBCErrCode
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCPriorityType_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCPriorityType.cpp
)

add_executable ( OBCPriorityType ${OBCPriorityType_SRC_FILES} )

# set library for each target
target_link_libraries( OBCPriorityType
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCProtocolType_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCProtocolType.cpp
)

add_executable ( OBCProtocolType ${OBCProtocolType_SRC_FILES} )

# set library for each target
target_link_libraries( OBCProtocolType
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)

SET ( OBCUdsResponseType_SRC_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/gtest_main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/testscripts/UT_GMOCK_OBCUdsResponseType.cpp
)

add_executable ( OBCUdsResponseType ${OBCUdsResponseType_SRC_FILES} )

# set library for each target
target_link_libraries( OBCUdsResponseType
	dl
	ionboardclient
	pthread
	rt
	binder
	cutils
	log
	autils
	corebase
	context
	serviceutils
	property
	iapplication
	-fprofile-arcs
	-ftest-coverage
	-fno-rtti
	gtest
	gcov
	gmock
)

SETUP_TARGET_FOR_COVERAGE(
	NAME vif_coverage
	EXECUTABLE vif
	DEPENDENCIES vif
	)
