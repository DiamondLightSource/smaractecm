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
    // axis number for smarpod 1=X 2=Y 3=Z 4=RX 5=RY 6=RZ
    int axisNum;
    int curPosition;
    int previousPosition;
    int previousDirection;
};


#endif /* SMARACTAXIS_H_ */
