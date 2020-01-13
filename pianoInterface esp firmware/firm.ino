#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "src/circularBuffer.h"
#include "src/lighting/lighting.h"
#include "src/m_error.h"
#include "src/m_constants.h"
#include "src/music.h"
#include "src/network.h"
#include "src/pinaoCom.h"
#include "src/settings.h"

TaskHandle_t taskA;
void PollThreadFunc(void *pvParameters);

unsigned long poll100Timer = 0;
void poll100();

void setup()
{
  Serial.begin(115200);

  settings::init();
  //settings::restoreDefaults();
  settings::loadSettings();
  settings::dumpToSerial();

  // Assuiming the USB shield is connected, there's no reason this should fail.
  bool USBSuccess = MIDI::initUSBHost();

  // Connect to strip and display the startup animation
  lights::init();
  lights::setAnimationMode(lights::AnimationMode::Startup);
  while (!lights::animationCompleted())
  {
    lights::updateAnimation();
  }

  lights::setAnimationMode(lights::AnimationMode::ColorfulIdle);
  InitBuffer();

  // if the MAX3421E didn't connect, don't do anything besides set up the
  // LEDs so that the error code can be displayed.
  if (USBSuccess)
  {
    // Start up the network connection
    network::beginConnection();

    // Spin up the second thread on core 1 which handles TCP and MIDI
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

    lights::setAnimationMode(lights::AnimationMode::KeyIndicateFade);
    //lights::setAnimationMode(lights::AnimationMode::KeyIndicateFade);
    MIDI::setLogicalLayerEnable(true);
  };

  if (network::waitForConnection())
  {
    network::startServer();
    PushEvent(e);
  }

  // THREAD 1 endless loop
  for (;;)
  {
    network::pollEvents();
    MIDI::pollMIDI();
  }
}

// THREAD 0 endless loop
void loop()
{
  // If there is an error, show the error code until reset
  if (isErrorLocked())
  {
    for (;;)
    {
      lights::updateAnimation();
    }
  }

  // Poll events
  {
    Event e;
    while (PopEvent(&e))
    {
      e.action();
    }
  }

  // call the slow poll
  unsigned long time;
  time = micros();
  if (time >= poll100Timer + (1000000 / 100))
  {
    poll100Timer = time;
    poll100();
  }

  //  update the animations as often as possible
  lights::updateAnimation();
}

void poll100()
{
  // Grab the MIDI mutex and update note press events
  MIDI::copyLogicalStateBuffer();
}