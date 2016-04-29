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
#include "IntegerParam.h"
#include "DoubleParam.h"
#include "EnumParam.h"

class EcmController : public asynMotorController
{
	// need to be friends with the derived classes of base smaractAxis
	// but otherwise mcsController only knows about the base
    friend class McsEcmAxis;
    friend class SmarpodAxis;

private:
    // Constants
    enum {RXBUFFERSIZE=200, TXBUFFERSIZE=200, RESBUFFERSIZE=30, CONNECTIONPOLL=10};
public:
    enum {NOAXIS=-1};
public:
    EcmController(const char *portName, int controllerNum,
            const char* serialPortName, int serialPortAddress, int numAxes,
            double movingPollPeriod, double idlePollPeriod);
    virtual ~EcmController();

    // Overridden from asynMotorController
    virtual asynStatus poll();
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

public:
    AsynParams asynParams;
    // New parameters
    IntegerParam paramVersionHigh;
    IntegerParam paramVersionLow;
    IntegerParam paramVersionBuild;
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
    asynUser* serialPortUser;
    int controllerNum;
    int connectionPollRequired;

private:
    /* Methods for use by the axes */
    bool sendReceive(const char* tx, const char* txTerminator,
            char* rx, size_t rxSize, const char* rxTerminator);
    bool command(const char* cmd, const char* rsp, int* a, int* b, int*c);
    bool command(const char* cmd, int p, int q, const char* rsp, int* a, int* b, int*c);
    bool command(const char* cmd, const char* rsp, int* a);
    bool command(const char* cmd, int p, const char* rsp, int* a);
    bool command(const char* cmd, int p, int q, const char* rsp, int* a);
    bool command(const char* cmd, int p, const char* rsp, int* a, int* b);
    bool command(const char* cmd, int p, const char* rsp, int* a, int* b, int* c);
    bool command(const char* cmd, int p, int q, const char* rsp, int* a, int* b);
    bool command(const char* cmd, int p, int q, int r, const char* rsp, int* a, int* b);
    bool command(const char* cmd, int p, int q, int r, int s, const char* rsp, int* a, int* b);
    /* Parameter notification methods */
    void onCalibrateSensor(TakeLock& takeLock, int list, int value);
    void onPowerSave(TakeLock& takeLock, int list, int value);
    void onPhysAxisChange(TakeLock& takeLock, int list, int value);
};


#endif /* ECMCONTROLLER_H_ */
