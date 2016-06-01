/*
 * smarpodAxis.cpp
 *
 *  Created on: 10 Feb 2016
 *      Author: hgv27681
 */

#include "smarpodAxis.h"
#include "ecmController.h"
#include <epicsThread.h>

/************
 * TODO
 * DONE use cr/lf
 * DONE homing / homed
 * DONE moving status
 * DONE stop
 * DONE get resolution correct
 * DONE velocity control
 * DONE Switching Unit number
 * pivot point setting (and mode)
 * sensor mode control (should be left default?)
 * DONE show error state on disconnect
 * CSS GUI - after testing with hardware
 * acceleration control??
 * 
 * from first tests :-
 *  DONE need to cope with errors before referencing - dont treat errors as disconect
 *  note speeds are linear actuator - move speed control to smarpod?
 *  DONE possibly allow moves during move - show only commanded axes moving ??
 * 
 * Bugs
 * loss of comms does not show in real hardware (does on sim?)
 * PINI of velocity and pivot point not working (on real HW)
 * getting tdir on zrotation axis (and others?) -- try doinf and increase on NTMF 
 *      for all axes
 *************/

SmarpodAxis::SmarpodAxis(EcmController* ctlr, int axisNum, Smarpod* smarpod,
        int smarpodAxis) :
        SmaractAxis(ctlr, axisNum), smarpod(smarpod), smarpodAxis(smarpodAxis)
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

    result = smarpod->move(smarpodAxis, position, relative, minVelocity,
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

    smarpod->connected(smarpodAxis);

    TakeLock takeLock(freeLock);
    setIntegerParam(controller->motorStatusCommsError_, 0);
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

/** Get status from the controller that needs polling
 */
bool SmarpodAxis::pollStatus(FreeLock& freeLock)
{
    // If axis not connected to a physical address, do nothing
    if (!isConnected())
        return false;

    bool result = true;
    double curPosition;
    bool homeStatus;
    int moving;
    int direction = 0;

    result = smarpod->getAxis(smarpodAxis, &curPosition, &moving, &homeStatus);

    if (result)
    {
        TakeLock takeLock(freeLock);
        setDoubleParam(controller->motorEncoderPosition_, (double) curPosition);
        setDoubleParam(controller->motorPosition_, (double) curPosition);
        setIntegerParam(controller->motorStatusDone_, (int) (moving == 0));
        setIntegerParam(controller->motorStatusMoving_, (int) (moving != 0));
        setIntegerParam(controller->motorStatusHomed_, (int) homeStatus);
        setDoubleParam(controller->motorVelocity_,
                (double) smarpod->getVelocity());
        // smarpod is in-operable before reference so set motor rec alarm
        setIntegerParam(controller->motorStatusProblem_, (int) !homeStatus);

        /* Use previous position and current position to calculate direction.*/
        if ((curPosition - previousPosition) > 0)
        {
            direction = 1;
        }
        else if (curPosition - previousPosition == 0.0)
        {
            direction = previousDirection;
        }
        else
        {
            direction = 0;
        }
        setIntegerParam(controller->motorStatusDirection_, direction);
        /*Store position to calculate direction for next poll.*/
        previousPosition = curPosition;
        previousDirection = direction;

        controller->paramMotorStatusHomed[smarpodAxis] = homeStatus != 0;

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
    // If axis not connected to a physical address, do nothing
    if (!isConnected())
        return asynSuccess;

    TakeLock takeLock(controller, /*alreadyTake=*/true);

    // Get information
    setIntegerParam(controller->motorStatusDone_, 0);
    setIntegerParam(controller->motorStatusMoving_, 1);
    setIntegerParam(controller->motorStatusDirection_, forwards);

    // Perform the move with the lock on since the protocol does
    // not return until the home complete or fails
    smarpod->home();

    // Do a poll now
    FreeLock freeLock(takeLock);
    pollStatus(freeLock);
    return asynSuccess;
}

/** Stop axis command.  Entered with the lock taken.
 * \param[in] acceleration The acceleration to use
 */
asynStatus SmarpodAxis::stop(double acceleration)
{
    // If axis not connected to a physical address, do nothing
    if (!isConnected())
        return asynSuccess;

    TakeLock takeLock(controller, /*alreadyTake=*/true);

    smarpod->stop();

    setIntegerParam(controller->motorStatusDone_, 1);
    setIntegerParam(controller->motorStatusMoving_, 0);

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
//    int oldYes = controller->paramCalibrateSensor[smarpodAxis];
//    if (oldYes == 0 && yes == 1)
//    {
//        int a;
//        int b;
//        controller->command("CS", physicalAxis(), "E", &a, &b);
//    }
}

