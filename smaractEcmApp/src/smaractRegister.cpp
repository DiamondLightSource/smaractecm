/*
 * smaractRegister.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include <string>
#include <map>
#include <iocsh.h>
#include <epicsExport.h>
#include "asynCommonSyncIO.h"
#include "ecmController.h"
#include "mcsEcmAxis.h"
#include "smarpodAxis.h"

/*******************************************************************************
 *
 *   The following functions have C linkage and can be called directly or from iocsh
 *
 *******************************************************************************/

extern "C"
{

static std::map<std::string, Smarpod*> theSmarpods;

/** Create a controller
 * \param[in] portName Asyn port name
 * \param[in] serialPortName Asyn port name of the serial port
 * \param[in] serialPortAddress Asyn address of the serial port (usually 0)
 * \param[in] numAxes Maximum number of axes
 * \param[in] movingPollPeriod The period at which to poll position while moving
 * \param[in] idlePollPeriod The period at which to poll position while not moving
 */
asynStatus ecmControllerConfig(const char *portName, const char* serialPortName,
        int serialPortAddress, int numAxes, int movingPollPeriod,
        int idlePollPeriod)
{
    EcmController* ctlr = new EcmController(portName, serialPortName,
            serialPortAddress, numAxes, movingPollPeriod / 1000.0,
            idlePollPeriod / 1000.0);
    ctlr = NULL;   // To avoid compiler warning
    return asynSuccess;
}

/** Create an ecm controlled mcs axis
 * param[in] ctlrName Asyn port name of the controller
 * param[in] axisNum The number of this axis
 */
asynStatus mcsEcmAxisConfig(const char* ctlrName, int axisNum, int rotary)
{
    asynStatus result = asynSuccess;
    EcmController* ctlr = (EcmController*) findAsynPortDriver(ctlrName);
    if (ctlr == NULL)
    {
        printf("mcsEcmAxisConfig: Could not find Ecm controller :\"%s\"\n",
                ctlrName);
        result = asynError;
    }
    else
    {
        McsEcmAxis* axis = new McsEcmAxis(ctlr, axisNum, rotary > 0);
        axis = NULL; // To avoid compiler warning
    }
    return result;
}

/** Create an smarpod
 * param[in] ctlrName Asyn port name of the controller
 * param[in] name for this smarpod (to be used in axis declarations)
 */
asynStatus smarpodConfig(const char* smarPodName, const char* ctlrName,
        double resolution)
{
    asynStatus result = asynSuccess;
    EcmController* ctlr = (EcmController*) findAsynPortDriver(ctlrName);
    if (ctlr == NULL)
    {
        printf("smarpodAxisConfig: Could not find Ecm controller :\"%s\"\n",
                ctlrName);
        result = asynError;
    }
    else
    {
        Smarpod* newpod = new Smarpod(ctlr, resolution);
        theSmarpods[smarPodName] = newpod;
    }
    return result;
}

/** Create an ecm controlled smarpod axis
 * param[in] ctlrName Asyn port name of the controller
 * param[in] axisNum The number of this axis
 */
asynStatus smarpodAxisConfig(const char* ctlrName, int axisNum,
        const char* smarPodName)
{
    asynStatus result = asynSuccess;
    EcmController* ctlr = (EcmController*) findAsynPortDriver(ctlrName);
    Smarpod* smar = theSmarpods[smarPodName];
    if (ctlr == NULL)
    {
        printf("smarpodAxisConfig: Could not find Ecm controller :\"%s\"\n",
                ctlrName);
        result = asynError;
    }
    else if (smar == NULL)
    {
        printf("smarpodAxisConfig: Could not find smarpod :\"%s\"\n",
                smarPodName);
        result = asynError;
    }
    else
    {
        SmarpodAxis* axis = new SmarpodAxis(ctlr, axisNum, smar);
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

static const iocshArg ecmControllerConfigArg0 =
{ "port name", iocshArgString };
static const iocshArg ecmControllerConfigArg1 =
{ "serial port name", iocshArgString };
static const iocshArg ecmControllerConfigArg2 =
{ "serial port address", iocshArgInt };
static const iocshArg ecmControllerConfigArg3 =
{ "number of axes", iocshArgInt };
static const iocshArg ecmControllerConfigArg4 =
{ "moving poll period (ms)", iocshArgInt };
static const iocshArg ecmControllerConfigArg5 =
{ "idle poll period (ms)", iocshArgInt };

static const iocshArg * const ecmControllerConfigArgs[] =
{ &ecmControllerConfigArg0, &ecmControllerConfigArg1, &ecmControllerConfigArg2,
        &ecmControllerConfigArg3, &ecmControllerConfigArg4,
        &ecmControllerConfigArg5 };
static const iocshFuncDef ecmControllerConfigDef =
{ "ecmControllerConfig", 6, ecmControllerConfigArgs };

static void ecmControllerConfigCallFunc(const iocshArgBuf *args)
{
    ecmControllerConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival,
            args[4].ival, args[5].ival);
}

static const iocshArg mcsEcmAxisConfigArg0 =
{ "controller port name", iocshArgString };
static const iocshArg mcsEcmAxisConfigArg1 =
{ "axis number", iocshArgInt };
static const iocshArg mcsEcmAxisConfigArg2 =
{ "a rotary stage", iocshArgInt };

static const iocshArg * const mcsEcmAxisConfigArgs[] =
{ &mcsEcmAxisConfigArg0, &mcsEcmAxisConfigArg1, &mcsEcmAxisConfigArg2 };
static const iocshFuncDef mcsEcmAxisConfigDef =
{ "mcsEcmAxisConfig", 3, mcsEcmAxisConfigArgs };

static void mcsEcmAxisConfigCallFunc(const iocshArgBuf *args)
{
    mcsEcmAxisConfig(args[0].sval, args[1].ival, args[2].ival);
}

static const iocshArg smarpodConfigArg0 =
{ "smarpod name", iocshArgString };
static const iocshArg smarpodConfigArg1 =
{ "controller port name", iocshArgString };
static const iocshArg smarpodConfigArg2 =
{ "smarpod axes resolution", iocshArgDouble };
static const iocshArg * const smarpodConfigArgs[] =
{ &smarpodConfigArg0, &smarpodConfigArg1, &smarpodConfigArg2 };
static const iocshFuncDef smarpodConfigDef =
{ "smarpodConfig", 3, smarpodConfigArgs };

static void smarpodConfigCallFunc(const iocshArgBuf *args)
{
    smarpodConfig(args[0].sval, args[1].sval, args[2].dval);
}

static const iocshArg smarpodAxisConfigArg0 =
{ "controller port name", iocshArgString };
static const iocshArg smarpodAxisConfigArg1 =
{ "axis number", iocshArgInt };
static const iocshArg smarpodAxisConfigArg2 =
{ "smarpod name", iocshArgString };

static const iocshArg * const smarpodAxisConfigArgs[] =
{ &smarpodAxisConfigArg0, &smarpodAxisConfigArg1, &smarpodAxisConfigArg2 };
static const iocshFuncDef smarpodAxisConfigDef =
{ "smarpodAxisConfig", 3, smarpodAxisConfigArgs };

static void smarpodAxisConfigCallFunc(const iocshArgBuf *args)
{
    smarpodAxisConfig(args[0].sval, args[1].ival, args[2].sval);
}

static void ecmRegister(void)
{
    iocshRegister(&ecmControllerConfigDef, ecmControllerConfigCallFunc);
    iocshRegister(&mcsEcmAxisConfigDef, mcsEcmAxisConfigCallFunc);
    iocshRegister(&smarpodConfigDef, smarpodConfigCallFunc);
    iocshRegister(&smarpodAxisConfigDef, smarpodAxisConfigCallFunc);
}
epicsExportRegistrar(ecmRegister);

