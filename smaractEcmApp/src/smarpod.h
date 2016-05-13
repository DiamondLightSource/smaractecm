/*
 * smarPod.h
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#ifndef SMARACTAPP_SRC_SMARPOD_H_
#define SMARACTAPP_SRC_SMARPOD_H_

#include "FreeLock.h"
#include "TakeLock.h"

#include <asynPortDriver.h>
#include "ecmController.h"

#define AXIS_COUNT 6

class Smarpod
{
public:
    Smarpod(EcmController* ctlr);
    virtual ~Smarpod();

    bool move(int axisNum, double position, int relative, double minVelocity,
            double maxVelocity, double acceleration);

public:
    bool pollStatus(FreeLock& freeLock);
    bool getCurrentPositions(bool setDemands=false);
    bool getPosition(int axisNum, int* curPosition);
    bool getStatus(int axisNum, int* status);
    bool getHomeStatus(int axisNum, int* homeStatus);

private:
    EcmController* ctlr;
    bool firstMove;
    double positions[AXIS_COUNT];
    double demandPositions[AXIS_COUNT];
};

#endif /* SMARACTAPP_SRC_SMARPOD_H_ */
