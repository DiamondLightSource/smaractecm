/*
 * smaractAxis.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smaractAxis.h"
#include "ecmController.h"

// TODO not sure if this class needs any non-virtual methods or properties
// to be decided once SmarpodAxis is implemented

SmaractAxis::SmaractAxis(EcmController* ctlr, int axisNum)
: asynMotorAxis::asynMotorAxis(ctlr, axisNum)
, controller(ctlr)
, axisNum(axisNum)
, curPosition(0)
, previousPosition(0)
, previousDirection(0)
{
}

SmaractAxis::~SmaractAxis()
{
}

/**
 * Get the number of the physical axis that this logical axis is connected to.
 */
int SmaractAxis::physicalAxis()
{
	return controller->paramPhysAxis[axisNum];
}

bool SmaractAxis::isConnected()
{
	return physicalAxis() != EcmController::NOAXIS;
}
