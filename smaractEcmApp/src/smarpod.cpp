/* smarPodUnit.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include <math.h>
#include <string.h>
#include <cfloat>
#include "smarpod.h"

#define MOVE_STATUS_MOVING 2
#define HOMING_TIMEOUT 20
#define MM_CONVERT 1000.0

// Parameter name definitions
const char* Smarpod::namePIVOT_POS_X = "PIVOT_POS_X";
const char* Smarpod::namePIVOT_POS_Y = "PIVOT_POS_Y";
const char* Smarpod::namePIVOT_POS_Z = "PIVOT_POS_Z";
const char* Smarpod::namePIVOT_TYPE = "PIVOT_MODE";
const char* Smarpod::nameVELOCITY = "VELOCITY";
const char* Smarpod::nameENCODER_MODE = "ENCODER_MODE";
#define NUM_PARAMS (&LAST_PARAM - &FIRST_PARAM - 1)

/*
 * Internally we hold all positions velocities in native ECM format
 * which is in meters or degrees
 * All public interfaces to this class deliver values in motor record 'counts'
 * which is determined by the resolution parameter
 *
 * A sensible setting for resolution is 10^-12 This means one step in
 * the motor record represents a 1nm move (or 10^-12 degrees for angle axes)
 * This also means that the mres should be set to .001 for egus = microns
 * or .000001 for egus = mm
 *
 */
Smarpod::Smarpod(const char* port, EcmController* ctlr, double resolution,
        int unit) :
        asynPortDriver(port, 1 /*maxAddr*/, NUM_PARAMS,
        asynInt32Mask | asynFloat64Mask | asynDrvUserMask | asynOctetMask,
        asynInt32Mask | asynFloat64Mask | asynOctetMask,
        ASYN_CANBLOCK, /*ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=0 */
        1, /*autoConnect*/0, /*default priority */0 /* stack size*/), ctlr(
                ctlr), referenced(false), moveStatus(0), resolution(resolution), unit(
                unit), velocity(0), getAxisStatus(false)
{
    int i;

    for (i = 0; i < AXIS_COUNT; i++)
    {
        positions[i] = demandPositions[i] = 0;
        moving[i] = false;
    }

    for (i = 0; i < DIMENSIONS_COUNT; i++)
    {
        pivotPositions[i] = 0;
    }

    createParam(namePIVOT_POS_X, asynParamFloat64, &indexPIVOT_POS_X);
    createParam(namePIVOT_POS_Y, asynParamFloat64, &indexPIVOT_POS_Y);
    createParam(namePIVOT_POS_Z, asynParamFloat64, &indexPIVOT_POS_Z);
    createParam(namePIVOT_TYPE, asynParamInt32, &indexPIVOT_TYPE);
    createParam(nameVELOCITY, asynParamFloat64, &indexVELOCITY);
    createParam(nameENCODER_MODE, asynParamInt32, &indexENCODER_MODE);
}

Smarpod::~Smarpod()
{
    // destructor stub
}

bool Smarpod::connected(int axisNum)
{
    bool ok = true;
    int intVal;
    double doubleVal;

    if (axisNum == 0)
    {
        ok &= ctlr->setUnit(unit);

        // read in properties that have no asyn parameter
        ok &= ctlr->command("ref?", NULL, 0, &referenced, 1, TIMEOUT);

        // read back in all parameters that are not polled
        ok &= ctlr->command("vel?", NULL, 0, &doubleVal, 1, TIMEOUT);
        doubleVal *= MM_CONVERT;
        setDoubleParam(indexVELOCITY, doubleVal);

        getPivots();

        ok &= ctlr->command("sen?", NULL, 0, &intVal, 1, TIMEOUT);
        setIntegerParam(indexENCODER_MODE, intVal);

        ok &= ctlr->command("pvm?", NULL, 0, &intVal, 1, TIMEOUT);
        setIntegerParam(indexPIVOT_TYPE, intVal);

        // read in positions and reset demands
        ok &= getCurrentPositions(true);

        callParamCallbacks();
    }

    return ok;
}

bool Smarpod::getPivots()
{
    bool ok = true;

    ok &= ctlr->command("piv?", NULL, 0, pivotPositions, DIMENSIONS_COUNT,
    TIMEOUT);
    for (int i = 0; i < DIMENSIONS_COUNT; i++)
    {
        pivotPositions[i] *= MM_CONVERT;
    }
    setDoubleParam(indexPIVOT_POS_X, pivotPositions[DIMX]);
    setDoubleParam(indexPIVOT_POS_Y, pivotPositions[DIMY]);
    setDoubleParam(indexPIVOT_POS_Z, pivotPositions[DIMZ]);

    return ok;
}

double Smarpod::getVelocity()
{
    return velocity / resolution;
}

/*************************************************************
 * Velocity is ignored here - it is set globally for a smarpod
 *************************************************************/
bool Smarpod::move(int axisNum, double position, int relative,
        double minVelocity, double maxVelocity, double acceleration)
{
    bool result = true;

    result &= ctlr->setUnit(unit);

    if (result)
    {
        if (relative)
            demandPositions[axisNum] += position * resolution;
        else
            demandPositions[axisNum] = position * resolution;

        result = ctlr->command("mov", demandPositions, AXIS_COUNT, NULL, 0,
        TIMEOUT);
        if (result)
        {
            moving[axisNum] = true;
        }
    }
    return result;
}

/****************************************************
 * This function should only be called after setUnit
 */
bool Smarpod::getCurrentPositions(bool setDemands)
{
    int i;
    bool result = true;

    result &= ctlr->command("pos?", NULL, 0, positions, AXIS_COUNT, TIMEOUT);
    if (setDemands)
    {
        for (i = 0; i < AXIS_COUNT; i++)
        {
            demandPositions[i] = positions[i];
        }
    }

    return true;
}

bool Smarpod::getAxis(int axisNum, double* curPosition, int* movingStatus,
        bool* homeStatus)
{
    bool result = true;

    // all the axes will poll this object - we only actually talk to the
    // controller for axis 0 and store the results for other axes queries
    if (axisNum == 0)
    {
        result &= ctlr->setUnit(unit);
        result &= ctlr->command("mst?", NULL, 0, &moveStatus, 1, TIMEOUT);
        result &= getCurrentPositions();
        getAxisStatus = result;

        if (moveStatus != MOVE_STATUS_MOVING)
        {
            for (int i = 0; i < AXIS_COUNT; moving[i++] = false)
                ;
        }
        else
        {
            for (int i = 0; i < AXIS_COUNT; i++)
            {
                if (fabs(positions[i] - demandPositions[i]) < 1E-9)
                    moving[i] = false;
            }
        }
    }

    *curPosition = positions[axisNum] / resolution;
    *movingStatus = moving[axisNum];
    *homeStatus = (bool) referenced;

    return getAxisStatus;
}

bool Smarpod::home()
{
    bool result = false;

    result &= ctlr->setUnit(unit);
    result = ctlr->command("ref", (int*) NULL, 0, (int*) NULL, 0,
    HOMING_TIMEOUT);
    result &= ctlr->command("ref?", NULL, 0, &referenced, 1, TIMEOUT);

    // read back current positions and reset demands
    result &= getCurrentPositions(true);

    return result;
}

bool Smarpod::stop()
{
    bool result = false;

    result &= ctlr->setUnit(unit);
    result = ctlr->command("stop", (int*) NULL, 0, (int*) NULL, 0, TIMEOUT);

    // read back current positions and reset demands
    result &= getCurrentPositions(true);

    return result;
}

/*********************************
 * Overrides for Asy Port Driver
 *
 *********************************/
asynStatus Smarpod::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    asynStatus result = asynSuccess;
    bool ok = true;
    int parameter = pasynUser->reason;

    if (parameter == indexENCODER_MODE)
    {
        ok &= ctlr->setUnit(unit);
        ok &= ctlr->command("sen", &value, 1, NULL, 0, TIMEOUT);
    }
    else if (parameter == indexPIVOT_TYPE)
    {
        ok &= ctlr->setUnit(unit);
        ok &= ctlr->command("pvm", &value, 1, NULL, 0, TIMEOUT);
    }

    // set the parameter and do call backs (base class does this)
    result = asynPortDriver::writeInt32(pasynUser, value);

    if (!ok)
    {
        result = asynError;
    }

    return result;
}

asynStatus Smarpod::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    asynStatus result = asynSuccess;
    bool ok = true;
    int parameter = pasynUser->reason;

    double meters = value / MM_CONVERT;

    if ((parameter == indexPIVOT_POS_X) | (parameter == indexPIVOT_POS_Y)
            | (parameter == indexPIVOT_POS_Z))
    {
        if (parameter == indexPIVOT_POS_X)
            pivotPositions[DIMX] = value;
        else if (parameter == indexPIVOT_POS_Y)
            pivotPositions[DIMY] = value;
        else if (parameter == indexPIVOT_POS_Z)
            pivotPositions[DIMZ] = value;

        for(int i=0; i < DIMENSIONS_COUNT; i++)
            pivotPositions[i] /= MM_CONVERT;
        ok &= ctlr->setUnit(unit);
        ok &= ctlr->command("piv", pivotPositions, DIMENSIONS_COUNT, NULL, 0,
        TIMEOUT);
        // make sure the controller was happy with this value
        getPivots();
    }
    else if (parameter == indexVELOCITY)
    {
        ok &= ctlr->setUnit(unit);
        ok &= ctlr->command("vel", &meters, 1, NULL, 0, TIMEOUT);
        // make sure the controller was happy with this value
        ok &= ctlr->command("vel?", NULL, 0, &meters, 1, TIMEOUT);
        value = meters * MM_CONVERT;
    }

    // set the parameter and do call backs (base class does this)
    result = asynPortDriver::writeFloat64(pasynUser, value);

    if (!ok)
    {
        result = asynError;
    }

    return result;
}

