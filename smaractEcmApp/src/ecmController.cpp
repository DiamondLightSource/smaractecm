/*
 * ecmController.cpp
 *
 *  Created on: Apr 2016
 *      Author: hgv27681
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <epicsThread.h>
#include "ecmController.h"
#include "asynOctetSyncIO.h"
#include "asynCommonSyncIO.h"
#include "TakeLock.h"
#include "FreeLock.h"
#define DEBUG 0

#define info_header_len 19  // used to remove "Versions:\n " from %info target

/*******************************************************************************
 *
 *   The PMC381 controller class
 *
 *******************************************************************************/

/** Constructor
 * \param[in] portName Asyn port name
 * \param[in] controllerNum The number (address) of the controller in the protocol
 * \param[in] commPortName Asyn port name of the comm port
 * \param[in] commPortAddress Asyn address of the comm port (usually 0)
 * \param[in] numAxes Maximum number of axes
 * \param[in] movingPollPeriod The period at which to poll position while moving
 * \param[in] idlePollPeriod The period at which to poll position while not moving
 */
EcmController::EcmController(const char* portName, const char* commPortName,
        int commPortAddress, int numAxes, double movingPollPeriod,
        double idlePollPeriod) :
        asynMotorController(portName, numAxes, /*numParams=*/50,
        /*interfaceMask=*/0, /*interruptMask=*/0,
        /*asynFlags=*/ASYN_MULTIDEVICE | ASYN_CANBLOCK, /*autoConnect=*/1,
        /*priority=*/0, /*stackSize=*/0), asynParams(this)
// New Parameters
                , paramVersion(&asynParams, "VERSION", ""), paramConnected(
                &asynParams, "CONNECTED", false), paramSystemId(&asynParams,
                "SYSTEM_ID", 0), paramActiveHold(&asynParams, "ACTIVE_HOLD", 0), paramCalibrateSensor(
                &asynParams, "CALIBRATE_SENSOR", 0,
                new IntegerParam::Notify<EcmController>(this,
                        &EcmController::onCalibrateSensor)), paramSine(
                &asynParams, "SINE", 0), paramCosine(&asynParams, "COSINE", 0), paramPowerSave(
                &asynParams, "POWER_SAVE", 0,
                new IntegerParam::Notify<EcmController>(this,
                        &EcmController::onPowerSave)), paramPhysAxis(
                &asynParams, "PHYS_AXIS",
                new IntegerParam::Notify<EcmController>(this,
                        &EcmController::onPhysAxisChange)), paramAxisConnected(
                &asynParams, "AXIS_CONNECTED")
// Existing (Base Class) Parameters
                , paramMotorStatusDone(&asynParams, motorStatusDone_), paramMotorStatusMoving(
                &asynParams, motorStatusMoving_), paramMotorStatusDirection(
                &asynParams, motorStatusDirection_), paramMotorStatusHighLimit(
                &asynParams, motorStatusHighLimit_), paramMotorStatusLowLimit(
                &asynParams, motorStatusLowLimit_), paramMotorStatusHasEncoder(
                &asynParams, motorStatusHasEncoder_), paramMotorStatusSlip(
                &asynParams, motorStatusSlip_), paramMotorStatusCommsError(
                &asynParams, motorStatusCommsError_), paramMotorStatusFollowingError(
                &asynParams, motorStatusFollowingError_), paramMotorStatusProblem(
                &asynParams, motorStatusProblem_), paramMotorStatusHomed(
                &asynParams, motorStatusHomed_), paramMotorVelocity(&asynParams,
                motorVelocity_), paramMotorEncoderPosition(&asynParams,
                motorEncoderPosition_), paramMotorMotorPosition(&asynParams,
                motorPosition_), controllerNum(controllerNum), connectionPollRequired(
                0)
{
    // Connect to the comm port
    if (pasynOctetSyncIO->connect(commPortName, commPortAddress, &commPortUser,
    NULL) != asynSuccess)
    {
        printf("ecmController: Failed to connect to comm port %s\n",
                commPortName);
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
    // Get current connection state
    if (paramConnected)
    {
        // Poll the axes
        bool anyOk = false;
        for (int i = 0; i < numAxes_; i++)
        {
            // Skip explicitly disabled axes
            if (this->paramPhysAxis[i] == NOAXIS)
            {
                this->paramAxisConnected[i] = false;
                continue;
            }

            SmaractAxis* pAxis = dynamic_cast<SmaractAxis*>(getAxis(i));
            if (pAxis != NULL)
            {
                // Try to poll twice if the first one fails
                bool thisOneOk = (pAxis->pollStatus(takeLock)
                        || pAxis->pollStatus(takeLock));
                this->paramAxisConnected[i] = thisOneOk;
                if (!thisOneOk)
                {
                    printf("Poll failed for axis %d\n", i);
                }
                anyOk = anyOk || thisOneOk;
            }
        }
        // Have we lost connection?
        if (!anyOk)
        {
            paramConnected = false;
        }
    }
    else
    {
        // Are we connected yet?
        bool ok = true;
        char version[RXBUFFERSIZE];

        ok = ok && command("%info device", version, RXBUFFERSIZE, TIMEOUT,
                        true);
        if (ok)
        {
            // Perform once only poll on the axes
            for (int i = 0; i < numAxes_; i++)
            {
                SmaractAxis* pAxis = dynamic_cast<SmaractAxis*>(getAxis(i));
                if (pAxis != NULL)
                {
                    pAxis->onceOnlyStatus(takeLock);
                }
            }
            if (ok)
            {
                // Everything ok, update parameters

                std::string str_version(version);

                // // Current compiler version not supporting regex!
                // const std::regex pattern("[a-zA-Z\s:]{15,}");
                // str_version = std::regex_replace(str_version, pattern, ", ");

                std::string str1 = "Device Serial Number: ";
                std::string str2 = "Device Product Code: ";
                std::string str3 = "Firmware Version: ";
                
                size_t pos0 = str_version.find(str1);
                size_t pos1 = str_version.find(str2, pos0);
                size_t pos2 = str_version.find(str3, pos1);
                
                size_t token1_begin = pos0 + str1.length();
                std::string serial_number = str_version.substr(token1_begin, pos1 - token1_begin);
                
                size_t token2_begin = pos1 + str2.length();
                std::string product_code = str_version.substr(token2_begin, pos2 - token2_begin);

                size_t token3_begin = pos2 + str3.length();
                std::string firmware = str_version.substr(token3_begin, std::string::npos - token3_begin);

                std::string stripped_version = "sn:" + serial_number + ",#:" + product_code + ",f:" + firmware;
                
                paramVersion = stripped_version;
                
                paramConnected = true;
            }
        }
    }

    if(!paramConnected)
    {
        // show connection error on axes
        for (int i = 0; i < numAxes_; i++)
        {
            SmaractAxis* pAxis = dynamic_cast<SmaractAxis*>(getAxis(i));
            if (pAxis != NULL)
            {
                pAxis->setIntegerParam(this->motorStatusCommsError_ , 1);
            }
        }
    }
    return asynSuccess;
}

/** Transmits a string to the controller and waits for a return result.
 *
 */
bool EcmController::sendReceive(const char* tx, char* rx, size_t rxSize,
        double timeout, bool multi_line)
{
    int eomReason;
    bool result = false;
    size_t bytesOut;
    size_t bytesIn;
    size_t bytesAll = 0;
#if DEBUG
    printf("Tx> %s\n", tx);
#endif
    pasynOctetSyncIO->flush(commPortUser);
    pasynOctetSyncIO->setInputEos(commPortUser, TERMINATOR, strlen(TERMINATOR));
    pasynOctetSyncIO->setOutputEos(commPortUser, TERMINATOR,
            strlen(TERMINATOR));
    asynStatus asynRes = pasynOctetSyncIO->writeRead(commPortUser, tx,
            strlen(tx), rx, rxSize, timeout, &bytesOut, &bytesIn, &eomReason);
    result = (asynRes == asynSuccess);

    if (result && multi_line)
    {
        bytesAll = bytesIn;
        while (bytesIn > 0)
        {
            pasynOctetSyncIO->read(commPortUser, rx + bytesAll,
                    rxSize - bytesAll, 0.1, &bytesIn, &eomReason);
            bytesAll += bytesIn;
        }
    }

#if DEBUG
    printf("Rx[%d]< %s\n", result, rx);
#endif

    return result;
}

/** parses the return from a command to determine if it contains a
 * result code
 * \param[in] rxbuffer the response from the controllerers
 * \return Returns result code or 0 if there is no result code
 */
int EcmController::parseReturnCode(const char* rxbuffer)
{
    int result = 0;

    if (rxbuffer[0] == '!')
    {
        result = atoi(rxbuffer + 1);
    }

    return result;
}

bool EcmController::setUnit(int unitNum)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    bool result = false;

    snprintf(txBuffer, TXBUFFERSIZE, "%%unit %d", unitNum);
    result = command(txBuffer, rxBuffer, RXBUFFERSIZE - 1, TIMEOUT);

    return result;
}

/** Transmits a command to the controller
 *   This function handles commands with floating point inputs and outputs
 * \param[in] cmd The command code to send
 * \param[in] inputs The parameters to the command
 * \param[in] inputCount number of command parameters
 * \param[out] outputs results buffer
 * \param[out] outputCount size of above and expected no. of outputs
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, double inputs[], int inputCount,
        double outputs[], int outputCount, double timeout)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    bool result = false;
    size_t txlen = 0;

    snprintf(txBuffer, TXBUFFERSIZE, "%s", cmd);
    for (int i = 0; i < inputCount; i++ && txlen < TXBUFFERSIZE)
    {
        txlen = strlen(txBuffer);
        snprintf(txBuffer + txlen, TXBUFFERSIZE - txlen, " %e", inputs[i]);
    }
    result = command(txBuffer, rxBuffer, RXBUFFERSIZE - 1, timeout);

    if (result)
    {
        int count = 0;
        char* p = rxBuffer;
        while (p < rxBuffer + RXBUFFERSIZE && count < outputCount)
        {
            char* end;
            outputs[count] = strtof(p, &end);
            if (end == p)
                break;
            count++;
            p = end;
        }
        result = (count == outputCount);
    }
    return result;

}

/** Transmits a command to the controller
 *   This function handles commands with integer inputs and outputs
 * \param[in] cmd The command code to send
 * \param[in] inputs The parameters to the command
 * \param[in] inputCount number of command parameters
 * \param[out] outputs results buffer
 * \param[out] outputCount size of above and expected no. of outputs
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, int inputs[], int inputCount,
        int outputs[], int outputCount, double timeout)
{
    char txBuffer[TXBUFFERSIZE];
    char rxBuffer[RXBUFFERSIZE];
    bool result = false;
    size_t txlen = 0;
    int count = 0;

    snprintf(txBuffer, TXBUFFERSIZE, "%s", cmd);
    for (int i = 0; i < inputCount; i++ && txlen < TXBUFFERSIZE)
    {
        txlen = strlen(txBuffer);
        snprintf(txBuffer + txlen, TXBUFFERSIZE - txlen, " %d", inputs[i]);
    }
    result = command(txBuffer, rxBuffer, RXBUFFERSIZE - 1, timeout);

    if (result)
    {
        char* p = rxBuffer;
        while (p < rxBuffer + RXBUFFERSIZE && count < outputCount)
        {
            char* end;
            outputs[count] = strtod(p, &end);
            if (end == p)
                break;
            count++;
            p = end;
        }
        result = (count == outputCount);
    }
    return result;
}

/** Transmits a command to the controller
 *   This function handles commands with floating point inputs and outputs
 * \param[in] cmd The command code to send
 * \param[out] output string results buffer
 * \param[in] output max size of above
 * \return Returns true for success
 */
bool EcmController::command(const char* cmd, char* output, int outsize,
        double timeout, bool multi_line)
{
    bool result = false;

    result = sendReceive(cmd, output, outsize - 1, timeout, multi_line);
    output[outsize - 1] = '\0';

    if (result)
    {
        int returnCode = parseReturnCode(output);
        if (returnCode != 0)
        {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "EcmController::command Error %d returned from command: %s\n",
                    returnCode, cmd);
            result = false;
        }
    }

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

asynStatus EcmController::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    TakeLock takeLock(this, /*alreadyTaken=*/true);

    // Base class
    asynStatus result = asynMotorController::writeInt32(pasynUser, value);

    // Delegate to param controller object
    asynParams.writeInt32(takeLock, pasynUser, value);

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
    if (physicalAxis < 0)
    {
        this->paramPhysAxis[logicalAxis] = NOAXIS;
        std::cout << "Logical axis " << logicalAxis << " disabled\n";
    }
    else
    {
        // If any other axes currently point to the requested physical
        //	axis, disable them (by setting to axis -1)
        int numLogicalAxes = this->numAxes_;
        for (int a = 0; a < numLogicalAxes; ++a)
        {
            if (a == logicalAxis)
                continue;

            if (this->paramPhysAxis[a] == physicalAxis)
            {
                this->paramPhysAxis[a] = NOAXIS;
                std::cout << "Logical axis " << a << " disabled\n";
            }
        }
        this->paramPhysAxis[logicalAxis] = physicalAxis;
        std::cout << "Logical axis " << logicalAxis << " set to physical axis "
                << physicalAxis << "\n";
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
    // int a;
    // int b;
    // this->command("SSE", value ? 2 : 1, "E", &a, &b);
}
