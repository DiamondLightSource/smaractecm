/*
 * smarPodUnit.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpod.h"

Smarpod::Smarpod(EcmController* ctlr) :
        ctlr(ctlr), firstMove(true)
{
    // constructor stub
}

Smarpod::~Smarpod()
{
    // destructor stub
}


bool Smarpod::connected(int axisNum)
{
    bool result = true;

    if(axisNum == 1)
    {
        result = getCurrentPositions(true);
    }

    return result;
}

bool Smarpod::move(int axisNum, double position, int relative,
        double minVelocity, double maxVelocity, double acceleration)
{
    bool result = true;

    // TODO - put in speed and acceleration control
    if (result)
    {
        if (relative)
            demandPositions[axisNum] += position;
        else
            demandPositions[axisNum] = position;

        result = ctlr->command("pos", demandPositions, AXIS_COUNT, NULL, 0);
    }
    return result;
}

bool Smarpod::getCurrentPositions(bool setDemands)
{
    int i;
    bool result = ctlr->command("pos?", NULL, 0, positions, AXIS_COUNT);
    if (setDemands)
    {
        for (i = 0; i < AXIS_COUNT; i++)
            demandPositions[i] = positions[i];
    }
    return result;
}

bool Smarpod::getAxis(int axisNum, double* curPosition, int* status,
        bool* homeStatus)
{
    // all the axes will poll this object - we only actually talk to the
    // controller for axis 1 and store the results for other axes queries

    if(axisNum == 1)
    {
        getCurrentPositions();
    }

    *curPosition = positions[axisNum];
    *status = 0;
    *homeStatus = false;

    return true;
}

