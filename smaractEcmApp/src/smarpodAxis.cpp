/*
 * smarpodAxis.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpodAxis.h"
#include "ecmController.h"
#include <epicsThread.h>

SmarpodAxis::SmarpodAxis(EcmController* ctlr, int axisNum, Smarpod* smarpod) :
        SmaractAxis(ctlr, axisNum),
        smarpod(smarpod)
{
}

SmarpodAxis::~SmarpodAxis()
{
}

/** Move axis command. Entered with the lock taken.
 * \param[in] position Where to move to
 * \param[in] relative Non-zero for a relative move
 * \param[in] minVelocity The minimum velocity during the move
 * \param[in] maxVelocity The maximum velocity during the move
 * \param[in] acceleration The acceleration to use
 */
asynStatus SmarpodAxis::move(double position, int relative, double minVelocity,
        double maxVelocity, double acceleration)
{
    bool result;

    result = smarpod->move(axisNum, position, relative, minVelocity,
            maxVelocity, acceleration);

    return result ? asynSuccess : asynError;
}

/** Get status from the controller that does not change.
 */
bool SmarpodAxis::onceOnlyStatus(FreeLock& freeLock)
{
    // If axis not connected to a physical address, do nothing
    if (!isConnected())
        return asynSuccess;

    smarpod->connected(axisNum);

    TakeLock takeLock(freeLock);
    setIntegerParam(controller->motorStatusHighLimit_, 0);
    setIntegerParam(controller->motorStatusLowLimit_, 0);
    setIntegerParam(controller->motorStatusHasEncoder_, 0);
    setDoubleParam(controller->motorVelocity_, 0.0);
    setIntegerParam(controller->motorStatusSlip_, 0);
    setIntegerParam(controller->motorStatusCommsError_, 0);
    setIntegerParam(controller->motorStatusFollowingError_, 0);
    setIntegerParam(controller->motorStatusProblem_, 0);

    return true;
}

/** Get status from the controller that needs polling frequently.
 *  We need to minimise the amount of traffic generated by the poll,
 *  so the less important parameters are only polled once in three.
 */
bool SmarpodAxis::pollStatus(FreeLock& freeLock)
{
    // If axis not connected to a physical address, do nothing
    if (!isConnected())
        return false;

    bool result = true;
    double curPosition;
    bool homeStatus;
    int status;

    result = smarpod->getAxis(axisNum, &curPosition, &status, &homeStatus);

    if (result)
    {
        TakeLock takeLock(freeLock);
        setDoubleParam(controller->motorEncoderPosition_, (double) curPosition);
        setDoubleParam(controller->motorPosition_, (double) curPosition);
        setIntegerParam(controller->motorStatusDone_,
                status == 3 || status == 0);
        setIntegerParam(controller->motorStatusMoving_,
                !(status == 3 || status == 0));

        controller->paramMotorStatusHomed[axisNum] = homeStatus != 0;

        this->callParamCallbacks();
    }

    return result;
}

/** Jog axis command
 * \param[in] minVelocity The minimum velocity during the move
 * \param[in] maxVelocity The maximum velocity during the move
 * \param[in] acceleration The acceleration to use
 */
asynStatus SmarpodAxis::moveVelocity(double minVelocity, double maxVelocity,
        double acceleration)
{
    // Jog not supported
    return asynError;
}

/** Home axis command.  Entered with the lock taken
 * \param[in] minVelocity The minimum velocity during the move
 * \param[in] maxVelocity The maximum velocity during the move
 * \param[in] acceleration The acceleration to use
 * \param[in] forwards Set to TRUE to home forwards
 */
asynStatus SmarpodAxis::home(double minVelocity, double maxVelocity,
        double acceleration, int forwards)
{
//    // If axis not connected to a physical address, do nothing
//    if (!isConnected())
//        return asynSuccess;
//
//    TakeLock takeLock(controller, /*alreadyTake=*/true);
//    int a;
//    int b;
//
//    // Get information
//    int activeHold = controller->paramActiveHold[axisNum];
//
//    setIntegerParam(controller->motorStatusDone_, 0);
//    setIntegerParam(controller->motorStatusMoving_, 1);
//    setIntegerParam(controller->motorStatusDirection_, forwards);
//
//    // Perform the move
//    FreeLock freeLock(takeLock);
//    controller->command("SCLS", physicalAxis(), (int) maxVelocity, "E", &a, &b);
//    controller->command("FRM", physicalAxis(), forwards ? 0 : 1,
//            60000 * activeHold, 1, "E", &a, &b);
//
//    // Wait for move to complete.
//    int status;
//    bool moving = true;
//    while (moving && controller->command("GS", physicalAxis(), "S", &a, &status))
//    {
//        moving = !(status == 3 || status == 0);
//    }
//
//    // Do a poll now
//    pollStatus(freeLock);
      return asynSuccess;
}

/** Stop axis command.  Entered with the lock taken.
 * \param[in] acceleration The acceleration to use
 */
asynStatus SmarpodAxis::stop(double acceleration)
{
//    // If axis not connected to a physical address, do nothing
//    if (!isConnected())
//        return asynSuccess;
//
//    TakeLock takeLock(controller, /*alreadyTake=*/true);
//
//    int a;
//    controller->command("S", physicalAxis(), "E", &a);
//
//    setIntegerParam(controller->motorStatusDone_, 1);
//    setIntegerParam(controller->motorStatusMoving_, 0);

    return asynSuccess;
}

/** Set position command
 * \param[in] position The current position of the motor
 */
asynStatus SmarpodAxis::setPosition(double position)
{
    // this is not possible for individual axis
    return asynError;
}

/** Calibrate the sensor attached to this channel.
 * \param[in] yes Controls whether calibration is performed
 */
void SmarpodAxis::calibrateSensor(TakeLock& takeLock, int yes)
{
//    // If axis not connected to a physical address, do nothing
//    if (!isConnected())
//        return;
//
//    int oldYes = controller->paramCalibrateSensor[axisNum];
//    if (oldYes == 0 && yes == 1)
//    {
//        int a;
//        int b;
//        controller->command("CS", physicalAxis(), "E", &a, &b);
//    }
}

