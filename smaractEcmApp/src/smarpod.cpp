/*
 * smarPodUnit.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpod.h"

Smarpod::Smarpod(EcmController* ctlr)
{
    // TODO Auto-generated constructor stub

}

Smarpod::~Smarpod()
{
    // TODO Auto-generated destructor stub
}

asynStatus Smarpod::move(int axisNum, double position, int relative,
        double minVelocity, double maxVelocity, double acceleration)
{
    return asynSuccess;
}
