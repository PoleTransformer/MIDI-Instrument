#include <MIDI.h>
#include <digitalWriteFast.h>
#include "pitches.h"

//Pin definitions:
const int stepPin_M2 = 5;
const int stepPin_M1 = 4;
const int dirPin_M2 = 2;
const int dirPin_M1 = 7;
const int disablePin2 = 8;
const int disablePin1 = 9;

const int floppyStep = 6;
const int floppyDirPin = 3;
const int floppyEnable = 13;

const int bassDrum = 11;
const int snareDrum = 10;
const int highDrum = 12;

//Variable definitions:
int pitchTarget[] = { -1, -1, -1, -1, -1, -1}; //Holds the target pitch for the pulse functions.
int pitchCurrent[] = { -1, -1, -1, -1, -1, -1}; //Holds the previous pitch value
int acceleration[] = { -1, -1, -1, -1, -1, -1}; //acceleration
int initialPitch[] = { -1, -1, -1, -1, -1, -1}; //Holds current midi pitch 0-127
int bendVal[] = { 0, 0, 0, 0, 0, 0};
int bendSens[] = {2, 2, 2, 2, 2, 2}; //2 semitones
bool motorDirections[] = {LOW, LOW, LOW, LOW, LOW}; //Directions of the motors.
bool motorStallMode[] = {LOW, LOW, LOW, LOW, LOW};
unsigned long prevStepMicros[] = {0, 0, 0, 0, 0, 0}; //last time

unsigned long prevDrum[] = {0, 0, 0};
int drumDuration[] = { -1, -1, -1};

bool floppyDir = LOW;
int floppyCount = 0;

int controlValue1 = -1;
int controlValue2 = -1;
MIDI_CREATE_DEFAULT_INSTANCE(); //use default MIDI settings

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  pinMode(stepPin_M1, OUTPUT);
  pinMode(stepPin_M2, OUTPUT);
  pinMode(dirPin_M1, OUTPUT);
  pinMode(dirPin_M2, OUTPUT);
  pinMode(disablePin1, OUTPUT);
  pinMode(disablePin2, OUTPUT);

  pinMode(floppyStep, OUTPUT);
  pinMode(floppyDirPin, OUTPUT);
  pinMode(floppyEnable, OUTPUT); //active low!
  pinMode(bassDrum, OUTPUT);
  pinMode(snareDrum, OUTPUT);
  pinMode(highDrum, OUTPUT);

  digitalWrite(dirPin_M1, LOW);
  digitalWrite(dirPin_M2, LOW);
  digitalWrite(disablePin1, HIGH);//Starts disabled!
  digitalWrite(disablePin2, HIGH);//Starts disabled!
  digitalWrite(floppyDirPin, HIGH);
  digitalWrite(floppyEnable, LOW);

  for (int i = 0; i < 80; i++) {
    digitalWriteFast(floppyStep, HIGH);
    digitalWriteFast(floppyStep, LOW);
    delay(5);
  }
  digitalWrite(floppyEnable, HIGH);
  digitalWrite(floppyDirPin, LOW);

  MIDI.begin(MIDI_CHANNEL_OMNI); //listen to all MIDI channels
  MIDI.setHandleNoteOn(handleNoteOn); //execute function when note on message is received
  MIDI.setHandleNoteOff(handleNoteOff); //execute function when note off message is received
  MIDI.setHandlePitchBend(myPitchBend);
  MIDI.setHandleControlChange(myControlChange);
  Serial.begin(115200); //Allows for serial MIDI communication.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  //Read midi messages and step each stepper at the correct speed (called frequently)
  MIDI.read();
  singleStep(1);
  singleStep(2);
  floppySingleStep(5);
  processDrum();
}

void processDrum() {
  if (drumDuration[0] != -1 && (micros() - prevDrum[0]) >= drumDuration[0]) { //bass drum
    drumDuration[0] = -1;
    digitalWriteFast(bassDrum, LOW);
  }
  if (drumDuration[1] != -1 && (micros() - prevDrum[1]) >= drumDuration[1]) { //snare drum
    drumDuration[1] = -1;
    digitalWriteFast(snareDrum, LOW);
  }
  if (drumDuration[2] != -1 && (micros() - prevDrum[2]) >= drumDuration[2]) { //high drum
    drumDuration[2] = -1;
    digitalWriteFast(highDrum, LOW);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel >= 1 && channel <= 5) && velocity > 0) //If it's a valid midi channel (1-4):
  {
    if (pitch == 51 || pitch == 52 || pitch == 53 || pitch == 54 || pitch == 55 || pitch == 56 || pitch == 57 || pitch == 58) //If it's a resonance note when rotating....
    {
      motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
    }
    else
    {
      motorStallMode[channel] = LOW;//No longer stalling itself.
    }

    if (pitch > 83) //84)//needs acceleration control over this point. It's not guaranteed to start.
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[70];//Force an acceleration from the highest pitch guaranteed to lock the rotor in first step.
      acceleration[channel] = 70;
    }
    else
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[pitch]; //No acceleration!
    }

    if (channel == 1) {
      digitalWriteFast(disablePin1, LOW);//Therefore the system needs to be switched on. It is faster to write each time than check the flags and write!
    }

    if (channel == 2) {
      digitalWriteFast(disablePin2, LOW);//Therefore the system needs to be switched on. It is faster to write each time than check the flags and write!
    }

    if (channel == 5) {
      digitalWriteFast(floppyEnable, LOW);
      pitchCurrent[5] = pitchVals[pitch];
    }

    initialPitch[channel] = pitch;
    prevStepMicros[channel] = micros();//Works better!?! BY A HUGE MARGIN TOO!?! (-300 helps the upper range a bit).
  }
  if (channel == 10 && velocity > 0) { //drum
    if (pitch == 35 || pitch == 36 || pitch == 39) { //bass drum
      if (velocity >= 64) {
        drumDuration[0] = map(velocity, 64, 127, 2350, 3000);
      }
      else {
        drumDuration[0] = map(velocity, 1, 63, 1, 1350);
      }
      digitalWriteFast(bassDrum, HIGH);
      prevDrum[0] = micros();
    }
    else if (pitch == 38 || pitch == 40) { //snare drum
      if (velocity >= 64) {
        drumDuration[1] = map(velocity, 64, 127, 2500, 3500);
      }
      else {
        drumDuration[1] = map(velocity, 1, 63, 1, 1500);
      }
      digitalWriteFast(snareDrum, HIGH);
      prevDrum[1] = micros();
    }
    else { //high drum
      drumDuration[2] = map(velocity, 1, 127, 1, 2000);
      digitalWriteFast(highDrum, HIGH);
      prevDrum[2] = micros();
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) //MIDI Note OFF Command
{
  if (channel >= 1 && channel <= 5) //If it's a valid midi channel (1-4):
  {
    if (channel == 1) //If all the steppers are flagged as off:
    {
      digitalWriteFast(disablePin1, HIGH);//Disable all the steppers!
    }

    if (channel == 2) //If all the steppers are flagged as off:
    {
      digitalWriteFast(disablePin2, HIGH);//Disable all the steppers!
    }

    if (channel == 5) {
      digitalWriteFast(floppyEnable, HIGH);
    }

    pitchTarget[channel] = -1;//set motor pitch to -1  (basically flags the situation later).
    pitchCurrent[channel] = -1;//Reset to a safe previous for the next next. Good practice.
    acceleration[channel] = -1;//acceleration reset
    initialPitch[channel] = -1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void singleStep(byte motorNum)
{
  if (((micros() - prevStepMicros[motorNum]) <= pitchCurrent[motorNum]) && initialPitch[motorNum] != -1)
  { //step when correct time has passed and the motor is at a nonzero speed
    //prevStepMicros[motorNum] += motorSpeeds[motorNum];//added accel
    //^^^ Instead of setting to Micros, this line allows for a feedback control that adjusts the total wait interval to account of "overshoot". BRILLIANT!
    if (motorNum == 1)
    {
      digitalWriteFast(stepPin_M1, HIGH);
      digitalWriteFast(stepPin_M1, LOW);
      if (motorStallMode[1]) //If we're in stalled motor mode...
      {
        motorDirections[1] = !motorDirections[1]; //Flip the stored direction of the motor.
        if (motorDirections[1]) {
          digitalWriteFast(dirPin_M1, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M1, LOW);
        }
      }
      if (bendVal[1] != 0) {
        processBend(1);
      }
    }
    else if (motorNum == 2)
    {
      digitalWriteFast(stepPin_M2, HIGH);
      digitalWriteFast(stepPin_M2, LOW);
      if (motorStallMode[2]) //If we're in stalled motor mode...
      {
        motorDirections[2] = !motorDirections[2]; //Flip the stored direction of the motor.
        if (motorDirections[2]) {
          digitalWriteFast(dirPin_M2, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M2, LOW);
        }
      }
      if (bendVal[2] != 0) {
        processBend(2);
      }
    }
    prevStepMicros[motorNum] = prevStepMicros[motorNum] + pitchCurrent[motorNum];//Keeps track of the last time a tick occurred for the next time.
    if (pitchCurrent[motorNum] > pitchTarget[motorNum]) //Not yet on target?
    {
      acceleration[motorNum]++;
      pitchCurrent[motorNum] = pitchVals[acceleration[motorNum]]; //Move the current closer to the target (acceleration control)!
    }
  }
}

void floppySingleStep(int channel) {
  if (((micros() - prevStepMicros[channel]) <= pitchCurrent[channel]) && initialPitch[channel] != -1) {
    digitalWriteFast(floppyStep, HIGH);
    digitalWriteFast(floppyStep, LOW);
    floppyCount++;
    if (floppyCount >= 80) {
      floppyDir = !floppyDir;
      if (floppyDir) {
        digitalWriteFast(floppyDirPin, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin, LOW);
      }
      floppyCount = 0;
    }
    if (bendVal[channel] != 0) {
      processBend(channel);
    }
    prevStepMicros[channel] = prevStepMicros[channel] + pitchCurrent[channel];
  }
}

void myPitchBend(byte channel, int val) {
  if (channel >= 1 && channel <= 5) {
    bendVal[channel] = val;
    if (val == 0) {
      pitchCurrent[channel] = pitchVals[initialPitch[channel]];
      pitchTarget[channel] = pitchCurrent[channel];
    }
  }
}

void processBend(int channel) {
  float note = initialPitch[channel];
  float bendF = bendVal[channel] * -1;
  bendF = bendF / 8192;
  bendF = bendF * bendSens[channel];
  bendF = bendF / 12;
  float bendFactor = pow(2, bendF);
  float frequency = pow(2, ((note - 69) / 12));
  pitchCurrent[channel] = 1000000 / (440 * frequency) * bendFactor;
  pitchTarget[channel] = pitchCurrent[channel];
}

void myControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 121) { //reset
    for (int i = 0; i < 6; i++) {
      initialPitch[i] = -1;
      pitchTarget[i] = -1;
      pitchCurrent[i] = -1;
      acceleration[i] = -1;
      bendVal[i] = 0;
      bendSens[i] = 2;
    }
    controlValue1 = -1;
    controlValue2 = -1;
  }
  if (firstByte == 101) { //pitch bend sensitivity
    controlValue1 = secondByte;
  }

  if (firstByte == 100) {
    controlValue2 = secondByte;
  }

  if (firstByte == 6 && controlValue1 == 0 && controlValue2 == 0 && channel >= 1 && channel <= 5) { //pitch bend sensitivity
    bendSens[channel] = secondByte;
    controlValue1 = -1;
    controlValue2 = -1;
  }
}
