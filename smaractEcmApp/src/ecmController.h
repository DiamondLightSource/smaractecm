/*
 * ecmController.h
 *
 *  Created on: Apr 2016
 *      Author: hgv27681
 */

#ifndef ECMCONTROLLER_H_
#define ECMCONTROLLER_H_

#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include "smaractAxis.h"
#include "AsynParams.h"
#include "StringParam.h"
#include "IntegerParam.h"
#include "DoubleParam.h"
#include "EnumParam.h"

#define TERMINATOR "\r\n"
#define TIMEOUT 20

class EcmController: public asynMotorController
{
    // need to be friends with the derived classes of base smaractAxis
    // but otherwise mcsController only knows about the base
    friend class SmarpodAxis;
    friend class McsEcmAxis;
    friend class Smarpod;

private:
    // Constants
    enum
    {
        RXBUFFERSIZE = 200,
        TXBUFFERSIZE = 200,
        RESBUFFERSIZE = 30,
        CONNECTIONPOLL = 10
    };
public:
    enum
    {
        NOAXIS = -1
    };
public:
    EcmController(const char *portName, const char* commPortName,
            int commPortAddress, int numAxes, double movingPollPeriod,
            double idlePollPeriod);
    virtual ~EcmController();

    // Overridden from asynMotorController
    virtual asynStatus poll();
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

public:
    AsynParams asynParams;
    // New parameters
    StringParam paramVersion;
    EnumParam<bool> paramConnected;
    IntegerParam paramSystemId;
    IntegerParam paramActiveHold;
    IntegerParam paramCalibrateSensor;
    IntegerParam paramSine;
    IntegerParam paramCosine;
    IntegerParam paramPowerSave;
    IntegerParam paramPhysAxis;
    IntegerParam paramAxisConnected;

    // Existing parameters
    EnumParam<bool> paramMotorStatusDone;
    EnumParam<bool> paramMotorStatusMoving;
    EnumParam<bool> paramMotorStatusDirection;
    EnumParam<bool> paramMotorStatusHighLimit;
    EnumParam<bool> paramMotorStatusLowLimit;
    EnumParam<bool> paramMotorStatusHasEncoder;
    EnumParam<bool> paramMotorStatusSlip;
    EnumParam<bool> paramMotorStatusCommsError;
    EnumParam<bool> paramMotorStatusFollowingError;
    EnumParam<bool> paramMotorStatusProblem;
    EnumParam<bool> paramMotorStatusHomed;
    DoubleParam paramMotorVelocity;
    DoubleParam paramMotorEncoderPosition;
    DoubleParam paramMotorMotorPosition;

private:
    /* Data */
    asynUser* commPortUser;
    int controllerNum;
    int connectionPollRequired;

private:
    /* Methods for use by the axes */
    bool sendReceive(const char* tx, char* rx, size_t rxSize,
            double timeout, bool multi_line=false);
    bool command(const char* rx, char* tx, int txSize,
            double timeout, bool multi_line=false);
    bool command(const char* cmd, double inputs[], int inputCount,
            double outputs[], int outputCount, double timeout );
    bool command(const char* cmd, int inputs[], int inputCount,
            int outputs[], int outputCount, double timeout);
    /* Parameter notification methods */
    void onCalibrateSensor(TakeLock& takeLock, int list, int value);
    void onPowerSave(TakeLock& takeLock, int list, int value);
    void onPhysAxisChange(TakeLock& takeLock, int list, int value);
    int parseReturnCode(const char* rxbuffer);
};

#endif /* ECMCONTROLLER_H_ */
