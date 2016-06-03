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

#define AXIS_COUNT 6
#define DIMENSIONS_COUNT 3
#define DIMX 0
#define DIMY 1
#define DIMZ 2
#define PORTNAME "Smarpod%d"
#define PORTNAME_MAX 20

class Smarpod: public asynPortDriver
{
public:
    Smarpod(const char* port, EcmController* ctlr, double resolution, int unit);
    virtual ~Smarpod();

    bool move(int axisNum, double position, int relative, double minVelocity,
            double maxVelocity, double acceleration);

public:
    bool connected(int axisNum);
    bool home();
    bool stop();
    bool getAxis(int axisNum, double* curPosition, int* movingStatus,
            bool* homeStatus);
    double getVelocity();
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

private:
    bool getCurrentPositions(bool setDemands = false);
    bool  getPivots();

private:
    EcmController* ctlr;
    double positions[AXIS_COUNT];
    double demandPositions[AXIS_COUNT];
    double pivotPositions[DIMENSIONS_COUNT];
    bool moving[AXIS_COUNT];
    int referenced;
    int moveStatus;
    double resolution;
    int unit;
    double velocity;
    bool getAxisStatus;

private:
    static const char* namePIVOT_POS_X;
    static const char* namePIVOT_POS_Y;
    static const char* namePIVOT_POS_Z;
    static const char* namePIVOT_TYPE;
    static const char* nameVELOCITY;
    static const char* nameENCODER_MODE;

    int FIRST_PARAM;
    int indexPIVOT_POS_X;
    int indexPIVOT_POS_Y;
    int indexPIVOT_POS_Z;
    int indexPIVOT_TYPE;
    int indexVELOCITY;
    int indexENCODER_MODE;
    int LAST_PARAM;
};

#endif /* SMARACTAPP_SRC_SMARPOD_H_ */
