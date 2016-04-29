TOP=../..

include $(TOP)/configure/CONFIG

# -------------------------------
# Build an Diamond Support Module
# -------------------------------

LIBRARY_IOC = smaractEcm

# smaractEcm.dbd will be installed into <top>/dbd
DBD += smaractEcm.dbd

smaract_DBD += base.dbd
smaract_DBD += asyn.dbd
smaract_DBD += motorSupport.dbd
smaract_DBD += smaractEcm.dbd

# The following are compiled and added to the support library
smaractEcm_SRCS += ecmController.cpp
smaractEcm_SRCS += mcsEcmAxis.cpp
smaractEcm_SRCS += smaractAxis.cpp
smaractEcm_SRCS += smaractEcmMain.cpp
smaractEcm_SRCS += smaractRegister.cpp
smaractEcm_SRCS += smarpodAxis.cpp
smaractEcm_SRCS += smarpod.cpp

# These two lines are needed for non-vxWorks builds, such as Linux
smaract_SRCS_DEFAULT += smaractMain.c
smaractEcm_SRCS_vxWorks += -nil-

# The following adds object code from base/src/vxWorks
smaractEcm_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# This line says that this IOC Application depends on the
# xxx Support Module
smaractEcm_LIBS += motor
smaractEcm_LIBS += asyn


include $(TOP)/configure/RULES
