/*
 * smarPodUnit.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpod.h"

#define MOVE_STATUS_MOVING 2
#define HOMING_TIMEOUT 20

/*
 * Internally we hold all positions velocities in native ECM format
 * which is in meters or degrees
 * All public interfaces to this class deliver values in 'counts'
 * which is set by the resolution parameter
 * A sensible setting for resolution is 10^-12 this means one step in
 * the motor record represents a 1nm move (or 10^-12 degrees for angle axes)
 *
 */
Smarpod::Smarpod(EcmController* ctlr, double resolution, int unit) :
        ctlr(ctlr), referenced(false), moveStatus(0), resolution(resolution), unit(
                unit), velocity(0), getAxisStatus(false)
{
    printf("### initializing smarpod with controller %s, res %e, unit %d\n",
            ctlr->portName, resolution, unit);
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        positions[i] = demandPositions[i] = 0;
    }
}

Smarpod::~Smarpod()
{
    // destructor stub
}

bool Smarpod::connected(int axisNum)
{
    bool result = true;

    if (axisNum == 0)
    {
        result &= ctlr->setUnit(unit);
        result &= getCurrentPositions(true);
        result &= ctlr->command("ref?", NULL, 0, &referenced, 1, TIMEOUT);
        result &= ctlr->command("vel?", NULL, 0, &velocity, 1, TIMEOUT);
    }

    return result;
}

double Smarpod::getVelocity()
{
    return velocity / resolution;
}

bool Smarpod::move(int axisNum, double position, int relative,
        double minVelocity, double maxVelocity, double acceleration)
{
    bool result = true;

    double setVel = maxVelocity * resolution;
    result &= ctlr->setUnit(unit);
    result &= ctlr->command("vel", &setVel, 1, NULL, 0, TIMEOUT);
    result &= ctlr->command("vel?", NULL, 0, &velocity, 1, TIMEOUT);

    // TODO - put in speed and acceleration control
    if (result)
    {
        if (relative)
            demandPositions[axisNum] += position * resolution;
        else
            demandPositions[axisNum] = position * resolution;

        result = ctlr->command("pos", demandPositions, AXIS_COUNT, NULL, 0,
        TIMEOUT);
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
            demandPositions[i] = positions[i];
        printf("### demands set\n");
    }
    return result;
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
    }

    *curPosition = positions[axisNum] / resolution;
    *movingStatus = moveStatus == MOVE_STATUS_MOVING;
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

