#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{
    void colorfulIdle(const float time)
    {
                for (unsigned int i = 0; i < _KEYCOUNT; i++)
        {
            float index = i * HSLRange_Over_KeyCount + time * 1000;
            while (index > 1530)
            {
                index -= 1530;
            }
            setColor(i, sweepHSL((unsigned int)(index)));
        }
    }
}