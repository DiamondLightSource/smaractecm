/*
 * smarPodUnit.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpod.h"

Smarpod::Smarpod(EcmController* ctlr) :
    ctlr(ctlr),
    firstMove(true)
{
    // constructor stub
}

Smarpod::~Smarpod()
{
    // destructor stub
}

bool Smarpod::move(int axisNum, double position, int relative,
        double minVelocity, double maxVelocity, double acceleration)
{
    bool result = true;
    if(firstMove)
    {
        // to avoid creep we keep the demand positions in member 'demandPositions'
        // but this is initialised with read backs here
        result = getCurrentPositions(true);
        firstMove = true;
    }

    // TODO - put in speed and acceleration control
    if(result)
    {
        if(relative)
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
    if(setDemands)
    {
        for(i = 0; i<AXIS_COUNT; i++)
            demandPositions[i] = positions[i];
    }
    return result;
}

bool Smarpod::getPosition(int axisNum, int* curPosition)
{
    return true;
}

bool Smarpod::getStatus(int axisNum, int* status)
{
    return true;
}

bool Smarpod::getHomeStatus(int axisNum, int* homeStatus)
{
    return true;
}

