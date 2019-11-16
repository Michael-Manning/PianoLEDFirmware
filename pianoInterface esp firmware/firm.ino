#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
// #include "task.h"
// #include "semphr.h"

#include "m_error.h"
bool errorLock = false; // Read only. Set by fatalError
#include "m_constants.h"
#include <Arduino.h>
#include "lighting.h"
#include "music.h"
#include "pinaoCom.h"
#include "network.h"
#include "circularBuffer.h"
#include "pinaoCom.h"

// modifide externally by network_h
PlayMode globalMode = PlayMode::Idle;

TaskHandle_t taskA;
void PollThreadFunc(void * pvParameters);

bool fatalError(ErrorCode errorCode, bool exec){
  if(exec)
  {
    return true;
  }
    errorLock = true;
    lights::displayErrorCode((byte)errorCode);
    lights::setAnimationMode(lights::AnimationMode::PulseError);
    return false;
};

void setup()
{
  //Serial.begin(115200);

  // Assuiming the USB shield is connected, there's no reason this should fail.
  bool USBSuccess = MIDI::initUSBHost();
   // Connect to strip and display the startup animation
  lights::init();
  lights::setAnimationMode(lights::AnimationMode::ColorfulIdle);
  InitBuffer();

  // if the MAX3421E didn't connect, don't do anything besides set up the 
  // LEDs so that the error code can be displayed.
  if(USBSuccess){
    network::beginConnection();

    xTaskCreatePinnedToCore(
        PollThreadFunc,
        "thread2",
        10000,
        NULL,
        1,
        &taskA,
        1);
  }
}


void PollThreadFunc(void *pvParameters)
{
  Event e;
  e.action = []() {
    lights::setAnimationMode(lights::AnimationMode::BlinkSuccess);
    while (!lights::animationCompleted())
    {
      lights::updateAnimation();
    }
    //lights::setAnimationMode(lights::AnimationMode::ColorfulIdle);
    lights::setAnimationMode(lights::AnimationMode::KeyIndicate);
  };

  if (network::waitForConnection())
  {
    network::startServer();
    PushEvent(e);
  }

  for(;;){
      network::pollEvents();
      MIDI::pollMIDI();
  }
}

void loop()
{
  if(errorLock){
    lights::updateAnimation();
    goto endLoopLabel;
  }

  // Poll events
  {
    Event e;
    while(PopEvent(&e)){
      e.action();
    }
  }

  //network::pollEvents();
  //MIDI::pollMIDI();

  switch (globalMode)
  {
  case PlayMode::Idle:
    break;
  case PlayMode::SongLoading:
    break;
  case PlayMode::Indicate:

    break;
  default:
    break;
  }
  
  lights::updateAnimation();


    // unsigned int notesChanged = MIDI::pollMIDI();
    // network::pollEvents();

    // if (mode == PlayMode::indicate)
    // {
    //     MIDI::NoteEvent *changed = MIDI::getRecentEvents();
    //     for (size_t i = 0; i < notesChanged; i++)
    //     {
    //         lights::setIndicate(changed[i].number, changed[i].state);
    //     }
    // }
    // // First note will not display anything in current state.
    // else if (mode == PlayMode::waiting && notesChanged > 0)
    // {
    //     MIDI::NoteEvent *changed = MIDI::getRecentEvents();
    //     music::songFrame *frame = music::currentFrame();

    //     // Case of all notes being pressed correctly
    //     if (music::checkFrameCompletion())
    //     {
    //         byte *note = frame->firstNote;
    //         while (note != frame->lastNote)
    //         {
    //             lights::setFadOut(*note, lights::indicateColorF);
    //             note++;
    //         }
    //         music::nextFrame();

    //         // Set all in frame colors to red
    //         // NOTE: WTF happens to notes that are pressed multiple times in a row with the fade out!!??
    //         lights::allOff();
    //         frame = music::currentFrame();
    //         note = frame->firstNote;
    //         while (note != frame->lastNote)
    //         {
    //             lights::setInFrame(*note, false); // This sets keys to indicate
    //             note++;
    //         }
    //     }
    //     // Case of some or none of the notes being pressed correctly
    //     else
    //     {
    //         for (size_t i = 0; i < notesChanged; i++)
    //         {
    //             byte *note = frame->firstNote;
    //             while (note != frame->lastNote)
    //             {
    //                 if (changed[i].number == *note)
    //                 {
    //                     lights::setInFrame(*note, changed[i].state);
    //                 }
    //                 note++;
    //             }
    //         }
    //     }
    // }
    endLoopLabel:;
}
