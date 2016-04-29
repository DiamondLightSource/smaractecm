/*
 * smaractAxis.h
 *
 *  Created on: Apr 2016
 *      Author: hgv27681
 */

#ifndef SMARACTAXIS_H_
#define SMARACTAXIS_H_

#include "asynMotorAxis.h"
#include "TakeLock.h"
#include "FreeLock.h"
class EcmController;

class SmaractAxis : public asynMotorAxis
{
public:
    SmaractAxis(EcmController* ctlr, int axisNum);
    virtual ~SmaractAxis();

    // Used by mcsController
    virtual void calibrateSensor(TakeLock& takeLock, int yes) = 0;
    virtual bool onceOnlyStatus(FreeLock& freeLock) = 0;
    virtual bool pollStatus(FreeLock& freeLock) = 0;

protected:
    int physicalAxis();
    bool isConnected();

protected:
    EcmController* controller;
    int axisNum;
    int curPosition;
};


#endif /* SMARACTAXIS_H_ */
