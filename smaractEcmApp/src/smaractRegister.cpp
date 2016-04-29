/*
 * smaractRegister.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include <iocsh.h>
#include <epicsExport.h>
#include "asynCommonSyncIO.h"
#include "ecmController.h"
#include "mcsEcmAxis.h"

/*******************************************************************************
*
*   The following functions have C linkage and can be called directly or from iocsh
*
*******************************************************************************/

extern "C"
{

/** Create a controller
 * \param[in] portName Asyn port name
 * \param[in] controllerNum The number (address) of the controller in the protocol
 * \param[in] serialPortName Asyn port name of the serial port
 * \param[in] serialPortAddress Asyn address of the serial port (usually 0)
 * \param[in] numAxes Maximum number of axes
 * \param[in] movingPollPeriod The period at which to poll position while moving
 * \param[in] idlePollPeriod The period at which to poll position while not moving
 */
asynStatus ecmControllerConfig(const char *portName, int controllerNum,
        const char* serialPortName, int serialPortAddress,
        int numAxes, int movingPollPeriod, int idlePollPeriod)
{
    EcmController* ctlr = new EcmController(portName, controllerNum,
            serialPortName, serialPortAddress, numAxes,
            movingPollPeriod/1000.0, idlePollPeriod/1000.0);
    ctlr = NULL;   // To avoid compiler warning
    return asynSuccess;
}

/** Create an axis
 * param[in] ctlrName Asyn port name of the controller
 * param[in] axisNum The number of this axis
 */
asynStatus mcsEcmAxisConfig(const char* ctlrName, int axisNum, int rotary)
{
    asynStatus result = asynSuccess;
    EcmController* ctlr = (EcmController*)findAsynPortDriver(ctlrName);
    if(ctlr == NULL)
    {
        printf("mcsEcmAxisConfig: Could not find MCS controller :\"%s\"\n", ctlrName);
        result = asynError;
    }
    else
    {
        McsEcmAxis* axis = new McsEcmAxis(ctlr, axisNum, rotary>0);
        axis = NULL; // To avoid compiler warning
    }
    return result;
}

}

/*******************************************************************************
*
*   Registering the IOC shell functions
*
*******************************************************************************/

static const iocshArg ecmControllerConfigArg0 = {"port name", iocshArgString};
static const iocshArg ecmControllerConfigArg1 = {"controller number", iocshArgInt};
static const iocshArg ecmControllerConfigArg2 = {"serial port name", iocshArgString};
static const iocshArg ecmControllerConfigArg3 = {"serial port address", iocshArgInt};
static const iocshArg ecmControllerConfigArg4 = {"number of axes", iocshArgInt};
static const iocshArg ecmControllerConfigArg5 = {"moving poll period (ms)", iocshArgInt};
static const iocshArg ecmControllerConfigArg6 = {"idle poll period (ms)", iocshArgInt};

static const iocshArg *const ecmControllerConfigArgs[] =
{
    &ecmControllerConfigArg0, &ecmControllerConfigArg1,
    &ecmControllerConfigArg2, &ecmControllerConfigArg3,
    &ecmControllerConfigArg4, &ecmControllerConfigArg5,
    &ecmControllerConfigArg6
};
static const iocshFuncDef ecmControllerConfigDef =
    {"ecmControllerConfig", 7, ecmControllerConfigArgs};

static void ecmControllerConfigCallFunc(const iocshArgBuf *args)
{
    ecmControllerConfig(args[0].sval, args[1].ival, args[2].sval, args[3].ival,
            args[4].ival, args[5].ival, args[6].ival);
}

static const iocshArg mcsEcmAxisConfigArg0 = {"controller port name", iocshArgString};
static const iocshArg mcsEcmAxisConfigArg1 = {"axis number", iocshArgInt};
static const iocshArg mcsEcmAxisConfigArg2 = {"a rotary stage", iocshArgInt};

static const iocshArg *const mcsEcmAxisConfigArgs[] =
{
    &mcsEcmAxisConfigArg0, &mcsEcmAxisConfigArg1, &mcsEcmAxisConfigArg2
};
static const iocshFuncDef mcsEcmAxisConfigDef =
    {"mcsEcmAxisConfig", 3, mcsEcmAxisConfigArgs};

static void mcsEcmAxisConfigCallFunc(const iocshArgBuf *args)
{
    mcsEcmAxisConfig(args[0].sval, args[1].ival, args[2].ival);
}

static void ecmRegister(void)
{
    iocshRegister(&ecmControllerConfigDef, ecmControllerConfigCallFunc);
    iocshRegister(&mcsEcmAxisConfigDef, mcsEcmAxisConfigCallFunc);
}
epicsExportRegistrar(ecmRegister);



