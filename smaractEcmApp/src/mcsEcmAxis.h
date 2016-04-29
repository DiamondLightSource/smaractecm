/*
 * mcsEcmAxis.h
 *
 *  Created on: Apr 2016
 *      Author: hgv27681
 */

#ifndef MCSAXIS_H_
#define MCSAXIS_H_

#include "smaractAxis.h"

class McsEcmAxis : public SmaractAxis
{
public:
    McsEcmAxis(EcmController* ctlr, int axisNum, bool rotary);
    virtual ~McsEcmAxis();

public:
    // Overridden from asynMotorAxis
    virtual asynStatus move(double position, int relative,
            double minVelocity, double maxVelocity, double acceleration);
    virtual asynStatus moveVelocity(double minVelocity,
            double maxVelocity, double acceleration);
    virtual asynStatus home(double minVelocity,
            double maxVelocity, double acceleration, int forwards);
    virtual asynStatus stop(double acceleration);
    virtual asynStatus setPosition(double position);

    // Overridden from smaractAxis
    virtual void calibrateSensor(TakeLock& takeLock, int yes);
    virtual bool onceOnlyStatus(FreeLock& freeLock);
    virtual bool pollStatus(FreeLock& freeLock);

private:
    bool rotary;
    enum {POLLPHASE0=0, POLLPHASE1, POLLPHASE2} pollPhase;
};


#endif /* MCSAXIS_H_ */
