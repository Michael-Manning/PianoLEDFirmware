#include <Arduino.h>
#include <stdint.h>

#include "lighting/lighting.h"
#include "m_error.h"
#include "serialDebug.h"

namespace
{
bool errorLock = false; // Read only. Set by fatalError
ErrorCode currentError = ErrorCode::NO_ERROR;
} // namespace

// Sets the error state if there is none and causes and error lock
// after the THREAD 0 loop completes
bool fatalError(ErrorCode errorCode, bool exec)
{
    if (exec)
    {
        return true;
    }
    // Don't want to overrite another error code
    if (errorLock == true)
    {
        return false;
    }
    errorLock = true;
    currentError = errorCode;
    lights::setRedLED(true);
    #ifdef ENABLE_SERIAL
    Serial.println("ErrorCode: " + String(static_cast<uint8_t>(errorCode)));
    #endif
    lights::displayErrorCode(static_cast<uint8_t>(errorCode));
    lights::setAnimationMode(lights::AnimationMode::PulseError);
    return false;
};

ErrorCode getCurrentError()
{
    return currentError;
}

// Whether an error has been set
bool isErrorLocked()
{
    return errorLock;
}