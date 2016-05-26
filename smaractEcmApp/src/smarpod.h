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
    Smarpod(EcmController* ctlr, double resolution, int unit);
    virtual ~Smarpod();

    bool move(int axisNum, double position, int relative, double minVelocity,
            double maxVelocity, double acceleration);

public:
    bool connected(int axisNum);
    bool home();
    bool stop();
    bool getAxis(int axisNum, double* curPosition, int* movingStatus,
            bool* homeStatus);
    double getVelocity();
private:
    bool getCurrentPositions(bool setDemands=false);

private:
    EcmController* ctlr;
    double positions[AXIS_COUNT];
    double demandPositions[AXIS_COUNT];
    int referenced;
    int moveStatus;
    double resolution;
    int unit;
    double velocity;
    bool getAxisStatus;
};

#endif /* SMARACTAPP_SRC_SMARPOD_H_ */
