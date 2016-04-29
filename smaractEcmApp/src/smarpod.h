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

class Smarpod
{
public:
    Smarpod(EcmController* ctlr);
    virtual ~Smarpod();

    asynStatus move(int axisNum, double position, int relative,
            double minVelocity, double maxVelocity, double acceleration);

public:
    bool pollStatus(FreeLock& freeLock);
};

#endif /* SMARACTAPP_SRC_SMARPOD_H_ */
