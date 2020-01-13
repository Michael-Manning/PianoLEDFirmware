#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../../pinaoCom.h"
#include "../../music.h"
#include "../../settings.h"
#include "../../m_error.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace
{
constexpr unsigned int maxWaves = 20;
float waveTimers[maxWaves];
unsigned int waveSpawnPositions[maxWaves];
unsigned int waveCount = 0;

float waveWidth = 3.0f;
float waveSpeed =  20.0f;

template<typename T>
constexpr T m_abs(T n)
{
    return n < 0 ? -n : n;
}
} // namespace

namespace animations
{

void wave(float deltaTime, bool firstFrame)
{
    if (firstFrame)
    {
        waveCount = 0;
        for (size_t i = 0; i < maxWaves; i++)
        {
            waveSpawnPositions[i] = 255;
        }
        
    }

    // reset everything
    setAll(Colors::Off);

    // get new waves
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        if (MIDI::getLogicalState(i + MIDI::ledNoteOffset))
        {
            if (waveCount < maxWaves)
            {
                unsigned int availableIndex = 0;
                while(true)
                {
                    if(waveSpawnPositions[availableIndex] == 255)
                    {
                        break;
                    }
                    availableIndex ++;
                    assert_fatal(availableIndex < maxWaves, ErrorCode::IMPOSSIBLE_INTERNAL);
                }
                
                waveSpawnPositions[availableIndex] = _KEYCOUNT - 1 - i;
                waveTimers[availableIndex] = 0.0f;
                waveCount++;
            }
            else
            {
                break;
            }
        }
    }

    // render waves
    for (size_t waveI = 0; waveI < waveCount; waveI++)
    {
        if(waveSpawnPositions[waveI] == 255)
        {
            continue;
        }
        float wavePos = waveTimers[waveI] * waveSpeed;
        waveTimers[waveI] += deltaTime;
        bool waveStillInFrame = false;
        for (unsigned int i = 0; i < _KEYCOUNT; i++)
        {
            float keyDist = m_abs((int)waveSpawnPositions[waveI] - (int)i);
            float keyInWaveDist = m_abs(keyDist - wavePos);
            if(keyInWaveDist < waveWidth)
            {
                addColor(i, Colors::Red * (keyInWaveDist / waveWidth));
                waveStillInFrame = true;
            }
        }
        if(!waveStillInFrame)
        {
            waveCount --;
            waveSpawnPositions[waveI] = 255; // LOOOOL
        }
    }
}

} // namespace animations