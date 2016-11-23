cmake_minimum_required(VERSION 3.5)
project(smaractEcm)

set(SUPPORT "/dls_sw/prod/R3.14.12.3/support")
set(WORK "/dls_sw/work/R3.14.12.3/support")
set(EPICS_BASE "/dls_sw/epics/R3.14.12.3/base")
set(TEMPLATE_TOP "${EPICS_BASE}/templates/makeBaseApp/top")
set(ASYN "${SUPPORT}/asyn/4-26")
set(MOTOR "${SUPPORT}/motor/6-9dls3")
set(ASYNAID "${SUPPORT}/asynaid/2-4")

include_directories(
    ${TEMPLATE_TOP}/include
    ${SUPPORT}/include
    ${WORK}/include
    ${ASYN}/include
    ${MOTOR}/include
    ${ASYNAID}/include
    ${EPICS_BASE}/include
    ${EPICS_BASE}/include/os/Linux)

set(SMARACTECM_SOURCE_FILES
    smaractEcmApp/src/smaractEcmMain.cpp
    smaractEcmApp/src/ecmController.h
    smaractEcmApp/src/smaractAxis.h
    smaractEcmApp/src/smarpodAxis.h
    smaractEcmApp/src/smarpod.h
    smaractEcmApp/src/ecmController.cpp
    smaractEcmApp/src/mcsEcmAxis.h
    smaractEcmApp/src/smaractAxis.cpp
    smaractEcmApp/src/smaractRegister.cpp
    smaractEcmApp/src/smarpodAxis.cpp
    smaractEcmApp/src/smarpod.cpp
    smaractEcmApp/src/mcsEcmAxis.cpp
    etc/makeIocs/O.Common/testM4/testM4App/src/testM4Main.cpp
    etc/makeIocs/O.Common/testM5/testM5App/src/testM5Main.cpp
    etc/makeIocs/O.Common/testSmaract/testSmaractApp/src/testSmaractMain.cpp
    iocs/testM4/testM4App/src/testM4Main.cpp
    iocs/testM4/testM4App/src/O.linux-x86_64/testM4_registerRecordDeviceDriver.cpp
    iocs/testM5/testM5App/src/testM5Main.cpp
    iocs/testM5/testM5App/src/O.linux-x86_64/testM5_registerRecordDeviceDriver.cpp
    iocs/testSmaract/testSmaractApp/src/testSmaractMain.cpp
    iocs/testSmaract/testSmaractApp/src/O.linux-x86_64/testSmaract_registerRecordDeviceDriver.cpp
)

add_library(smaractEcm_source ${SMARACTECM_SOURCE_FILES})
add_custom_target(smaractEcm_build
    COMMAND $(MAKE) -C /home/hgv27681/R3.14.12.3/support/smaractEcm
    SOURCES ${SMARACTECM_SOURCE_FILES})
