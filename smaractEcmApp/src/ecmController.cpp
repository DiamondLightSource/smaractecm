/*
 * ecmController.cpp
 *
 *  Created on: Apr 2016
 *      Author: hgv27681
 */

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <epicsThread.h>
#include "ecmController.h"
#include "asynOctetSyncIO.h"
#include "asynCommonSyncIO.h"
#include "TakeLock.h"
#include "FreeLock.h"
#define DEBUG 0

/*******************************************************************************
*
*   The PMC381 controller class
*
*******************************************************************************/

/** Constructor
 * \param[in] portName Asyn port name
 * \param[in] controllerNum The number (address) of the controller in the protocol
 * \param[in] serialPortName Asyn port name of the serial port
 * \param[in] serialPortAddress Asyn address of the serial port (usually 0)
 * \param[in] numAxes Maximum number of axes
 * \param[in] movingPollPeriod The period at which to poll position while moving
 * \param[in] idlePollPeriod The period at which to poll position while not moving
 */
EcmController::EcmController(const char* portName, int controllerNum,
        const char* serialPortName, int serialPortAddress, int numAxes,
        double movingPollPeriod, double idlePollPeriod)
: asynMotorController(portName, numAxes, /*numParams=*/50,
		/*interfaceMask=*/0, /*interruptMask=*/0,
		/*asynFlags=*/ASYN_MULTIDEVICE | ASYN_CANBLOCK, /*autoConnect=*/1,
		/*priority=*/0, /*stackSize=*/0)
, asynParams(this)
// New Parameters
, paramVersionHigh(&asynParams, "VERSION_HIGH", 0)
, paramVersionLow(&asynParams, "VERSION_LOW", 0)
, paramVersionBuild(&asynParams, "VERSION_BUILD", 0)
, paramConnected(&asynParams, "CONNECTED", false)
, paramSystemId(&asynParams, "SYSTEM_ID", 0)
, paramActiveHold(&asynParams, "ACTIVE_HOLD", 0)
, paramCalibrateSensor(&asynParams, "CALIBRATE_SENSOR", 0,
		new IntegerParam::Notify<EcmController>(this, &EcmController::onCalibrateSensor))
, paramSine(&asynParams, "SINE", 0)
, paramCosine(&asynParams, "COSINE", 0)
, paramPowerSave(&asynParams, "POWER_SAVE", 0,
		new IntegerParam::Notify<EcmController>(this, &EcmController::onPowerSave))
, paramPhysAxis(&asynParams, "PHYS_AXIS",
		new IntegerParam::Notify<EcmController>(this, &EcmController::onPhysAxisChange))
, paramAxisConnected(&asynParams, "AXIS_CONNECTED")
// Existing (Base Class) Parameters
, paramMotorStatusDone(&asynParams, motorStatusDone_)
, paramMotorStatusMoving(&asynParams, motorStatusMoving_)
, paramMotorStatusDirection(&asynParams, motorStatusDirection_)
, paramMotorStatusHighLimit(&asynParams, motorStatusHighLimit_)
, paramMotorStatusLowLimit(&asynParams, motorStatusLowLimit_)
, paramMotorStatusHasEncoder(&asynParams, motorStatusHasEncoder_)
, paramMotorStatusSlip(&asynParams, motorStatusSlip_)
, paramMotorStatusCommsError(&asynParams, motorStatusCommsError_)
, paramMotorStatusFollowingError(&asynParams, motorStatusFollowingError_)
, paramMotorStatusProblem(&asynParams, motorStatusProblem_)
, paramMotorStatusHomed(&asynParams, motorStatusHomed_)
, paramMotorVelocity(&asynParams, motorVelocity_)
, paramMotorEncoderPosition(&asynParams, motorEncoderPosition_)
, paramMotorMotorPosition(&asynParams, motorPosition_)
, controllerNum(controllerNum)
, connectionPollRequired(0)
{
    // Connect to the serial port
    if(pasynOctetSyncIO->connect(serialPortName, serialPortAddress,
            &serialPortUser, NULL) != asynSuccess)
    {
        printf("ecmController: Failed to connect to serial port %s\n",
                serialPortName);
    }

    // Create the poller thread
    startPoller(movingPollPeriod, idlePollPeriod, /*forcedFastPolls-*/10);
}

/** Destructor
 */
EcmController::~EcmController()
{
}

/** Polls the controller. Entered with the lock taken.
 */
asynStatus EcmController::poll()
{
	TakeLock takeLock(this, /*alreadyTaken=*/true);
	FreeLock freeLock(takeLock);
    // Get current connection state
    if(paramConnected)
    {
        // Poll the axes
		bool anyOk = false;
		for(int i=0; i<numAxes_; i++)
		{
			// Skip explicitly disabled axes
			if(this->paramPhysAxis[i] == NOAXIS)
			{
				this->paramAxisConnected[i] = false;
				continue;
			}

			SmaractAxis* pAxis = dynamic_cast<SmaractAxis*>(getAxis(i));
			if(pAxis != NULL)
			{
				// Try to poll twice if the first one fails
				bool thisOneOk = (pAxis->pollStatus(freeLock) || pAxis->pollStatus(freeLock));
				this->paramAxisConnected[i] = thisOneOk;
				if(!thisOneOk)
				{
					printf("Poll failed for axis %d\n", i);
				}
				anyOk = anyOk || thisOneOk;
			}
		}
        // Have we lost connection?
        if(!anyOk)
        {
        	TakeLock again(freeLock);
        	paramConnected = false;
        }
    }
    else
    {
        // Are we connected yet?
        bool ok = true;
        int verHigh;
        int verLow;
        int verBuild;
        int sysId;
        ok = ok && command("GIV", "IV", &verHigh, &verLow, &verBuild);
        if(ok)
        {
            ok = ok && command("GSI", "ID", &sysId);
            // Perform once only poll on the axes
            for(int i=0; i<numAxes_; i++)
            {
                SmaractAxis* pAxis = dynamic_cast<SmaractAxis*>(getAxis(i));
                if(pAxis != NULL)
                {
                    pAxis->onceOnlyStatus(freeLock);
                }
            }
            if(ok)
            {
                // Everything ok, update parameters
                TakeLock again(freeLock);
                paramVersionHigh = verHigh;
                paramVersionLow = verLow;
                paramVersionBuild = verBuild;
                paramSystemId = sysId;
                paramConnected = true;
            }
        }
    }
    return asynSuccess;
}

/** Transmits a string to the controller and waits for a return result.
 *
 */
bool EcmController::sendReceive(const char* tx, const char* txTerminator,
        char* rx, size_t rxSize, const char* rxTerminator)
{
    int eomReason;
    size_t bytesOut;
    size_t bytesIn;
#if DEBUG
    printf("Tx> %s\n", tx);
#endif
    pasynOctetSyncIO->flush(serialPortUser);
    pasynOctetSyncIO->setInputEos(serialPortUser, rxTerminator, strlen(rxTerminator));
    pasynOctetSyncIO->setOutputEos(serialPortUser, txTerminator, strlen(txTerminator));
    asynStatus result = pasynOctetSyncIO->writeRead(serialPortUser, tx, strlen(tx),
            rx, rxSize, /*timeout=*/1.0, &bytesOut, &bytesIn, &eomReason);
    // JAT: I obviously had some trouble with the reliability of the comms but this
    //      retry solution is now causing problems.  So I've removed the retry and
    //      extended the timeout of the first writeRead from 0.1 seconds.
    //if(result != asynSuccess)
    //{
    //    pasynOctetSyncIO->flush(serialPortUser);
    //    result = pasynOctetSyncIO->writeRead(serialPortUser, tx, strlen(tx),
    //            rx, rxSize, /*timeout=*/0.1, &bytesOut, &bytesIn, &eomReason);
    //}
#if DEBUG
    printf("Rx[%d]< %s\n", result, rx);
#endif
    return result == asynSuccess;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \param[out] c The third result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, const char* rsp, int* a, int* b, int* c)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s", cmd);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b, c) == 3;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] q The second parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, int q, const char* rsp, int* a, int* b)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d,%d", cmd, p, q);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b) == 2;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] q The second parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \param[out] c The third result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, int q, const char* rsp, int* a, int* b, int*c)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d,%d", cmd, p, q);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b, c) == 3;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] q The second parameter integer
 * \param[in] r The third parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, int q, int r, const char* rsp, int* a, int* b)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d,%d,%d", cmd, p, q, r);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b) == 2;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] q The second parameter integer
 * \param[in] r The third parameter integer
 * \param[in] s The fourth parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, int q, int r, int s, const char* rsp, int* a, int* b)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d,%d,%d,%d", cmd, p, q, r, s);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b) == 2;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, const char* rsp, int* a)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s", cmd);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a) == 1;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, const char* rsp, int* a)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d", cmd, p);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a) == 1;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] q The second parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, int q, const char* rsp, int* a)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d,%d", cmd, p, q);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a) == 1;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, const char* rsp, int* a, int* b)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d", cmd, p);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b) == 2;
    }
    return result;
}

/** Transmits a command to the controller
 * \param[in] cmd The command code to send
 * \param[in] p The first parameter integer
 * \param[in] rsp The response code to expect
 * \param[out] a The first result integer
 * \param[out] b The second result integer
 * \param[out] c The third result integer
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int p, const char* rsp, int* a, int* b, int*c)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    char rxFormat[RXBUFFERSIZE];
    bool result = false;
    sprintf(txBuffer, ":%s%d", cmd, p);
    result = sendReceive(txBuffer, "\n", rxBuffer, RXBUFFERSIZE-1, "\n");
    rxBuffer[RXBUFFERSIZE-1] = '\0';
    if(result)
    {
        sprintf(rxFormat, ":%s%%d,%%d,%%d", rsp);
        result = sscanf(rxBuffer, rxFormat, a, b, c) == 3;
    }
    return result;
}

/** An integer parameter has been written
 * \param[in] pasynUser Handle of the user writing the paramter
 * \param[in] value The value written
 */
asynStatus EcmController::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	TakeLock takeLock(this, /*alreadyTaken=*/true);

	// Base class
    asynStatus result = asynMotorController::writeInt32(pasynUser, value);

    // Delegate to param controller object
    asynParams.writeInt32(takeLock, pasynUser, value);

    return result;
}

asynStatus EcmController::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	TakeLock takeLock(this, /*alreadyTaken=*/true);

	// Base class
    asynStatus result = asynMotorController::writeFloat64(pasynUser, value);

    // Delegate to param controller object
    asynParams.writeFloat64(takeLock, pasynUser, value);

    return result;
}

/** Parameter change handler
 */
void EcmController::onPhysAxisChange(TakeLock& takeLock, int list, int value)
{
	int logicalAxis = list;
	int physicalAxis = value;

	// If the requested physical axis number is out of range, set
	//	the value to -1 (which = no axis).
	if(physicalAxis<0)
	{
		this->paramPhysAxis[logicalAxis] = NOAXIS;
		std::cout << "Logical axis " << logicalAxis << " disabled\n";
	}
	else
	{
		// If any other axes currently point to the requested physical
		//	axis, disable them (by setting to axis -1)
		int numLogicalAxes = this->numAxes_;
		for(int a=0; a<numLogicalAxes; ++a)
		{
			if(a == logicalAxis) continue;

			if(this->paramPhysAxis[a] == physicalAxis)
			{
				this->paramPhysAxis[a] = NOAXIS;
				std::cout << "Logical axis " << a << " disabled\n";
			}
		}
		this->paramPhysAxis[logicalAxis] = physicalAxis;
		std::cout << "Logical axis " << logicalAxis << " set to physical axis " << physicalAxis << "\n";
	}
}


/** Parameter change handler
 */
void EcmController::onCalibrateSensor(TakeLock& takeLock, int list, int value)
{
    SmaractAxis* axis = dynamic_cast<SmaractAxis*>(getAxis(list));
    axis->calibrateSensor(takeLock, value);
}

/** Parameter change handler
 */
void EcmController::onPowerSave(TakeLock& takeLock, int list, int value)
{
    // Change the sensor power save mode of the controller
    int a;
    int b;
    this->command("SSE", value ? 2 : 1, "E", &a, &b);
}
