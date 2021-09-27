#include <MIDI.h>
#include <digitalWriteFast.h>
#include "pitches.h"

#define floppyStep11 2 //name + channel + floppy#
#define floppyDir11 3
#define floppyStep12 4
#define floppyDir12 5
#define floppyStep13 6
#define floppyDir13 7

bool currentDir[] = {0, 0, 0};
int currentPitch[] = { -1, -1};
byte prevPitch[] = {0, 0};
float bendFactor[] = {1, 1};
unsigned long previous[] = {0, 0};
unsigned long previousMillis[] = {0, 0};
MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  for (int i = 2; i <= 13; i++) {
    pinMode(i, OUTPUT);
  }
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  Serial.begin(115200);
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (channel == 1 && velocity > 0) {
    prevPitch[channel] = pitch;
    if (pitch > 67) pitch -= 12;
    currentPitch[channel] = pitchVals[pitch];
    previous[channel] = micros();
    previousMillis[channel] = millis();
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if (channel == 1) {
    if (prevPitch[channel] == pitch) {
      currentPitch[channel] = -1;
      prevPitch[channel] = 0;
    }
  }
}

void stepFloppy(byte channel) {
  if (currentPitch[channel] != -1 && ((micros() - previous[channel]) <= currentPitch[channel])) {
    floppySingleStep(1, floppyStep11, floppyDir11, 1);
    if (millis() - previousMillis[channel] >= 30) { //envelope simulation
      if(millis() - previousMillis[channel] <= 200) {
        floppySingleStep(1, floppyStep12, floppyDir12, 2);
      }
    }
    previous[channel] += currentPitch[channel] * bendFactor[channel];
  }
}

void floppySingleStep(byte channel, byte floppyStep, byte floppyDir, byte floppyNum) {
  digitalWriteFast(floppyStep, HIGH);
  digitalWriteFast(floppyStep, LOW);
  currentDir[floppyNum] = !currentDir[floppyNum];
  if (currentDir[floppyNum]) {
    digitalWriteFast(floppyDir, HIGH);
  }
  else {
    digitalWriteFast(floppyDir, LOW);
  }
}

void handlePitchBend(byte channel, int bend) {
  if (channel == 1) {
    float bendF = bend;
    bendF /= 8192;
    bendF *= -2;
    bendF /= 12;
    bendFactor[channel] = pow(2, bendF);
  }
}

void loop() {
  MIDI.read();
  stepFloppy(1);
}
