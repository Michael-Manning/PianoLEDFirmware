#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../../pinaoCom.h"
#include "../../music.h"
#include "../../settings.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{

void keyIndicate()
{
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            bool state = MIDI::getNoteState(i + MIDI::ledNoteOffset);
            if (state)
            {
                if (music::isBlackNote(i + MIDI::ledNoteOffset))
                {
                    setColor(_KEYCOUNT - 1 - i, settings::getColorSetting(settings::Colors::IndicateBlack));
                }
                else
                {
                    setColor(_KEYCOUNT - 1 - i, settings::getColorSetting(settings::Colors::IndicateWhite));
                }
            }
            else if (!music::isBlackNote(i + MIDI::ledNoteOffset))
            {
                setColor(_KEYCOUNT - 1 - i, settings::getColorSetting(settings::Colors::Ambiant));
            }
            else
            {
                setColor(_KEYCOUNT - 1 - i, Colors::Off);
            }
        }
}

} // namespace animations