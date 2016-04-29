/*
 * smarpodAxis.h
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#ifndef SMARACTAPP_SRC_SMARPODAXIS_H_
#define SMARACTAPP_SRC_SMARPODAXIS_H_

#include "smaractAxis.h"
#include "smarpod.h"

class SmarpodAxis: public SmaractAxis
{
public:
    SmarpodAxis(EcmController* ctlr, int axisNum,  Smarpod* smarpod);
    virtual ~SmarpodAxis();

public:
    // Overridden from asynMotorAxis
    virtual asynStatus move(double position, int relative, double minVelocity,
            double maxVelocity, double acceleration);
    virtual asynStatus moveVelocity(double minVelocity, double maxVelocity,
            double acceleration);
    virtual asynStatus home(double minVelocity, double maxVelocity,
            double acceleration, int forwards);
    virtual asynStatus stop(double acceleration);
    virtual asynStatus setPosition(double position);

    // Overridden from smaractAxis
    virtual void calibrateSensor(TakeLock& takeLock, int yes);
    virtual bool onceOnlyStatus(FreeLock& freeLock);
    virtual bool pollStatus(FreeLock& freeLock);

private:
    Smarpod* smarpod;
};

#endif /* SMARACTAPP_SRC_SMARPODAXIS_H_ */
