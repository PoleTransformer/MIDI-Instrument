#include "pitches.h"
#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))

//Stepper Motors:
#define stepPin_M1 22 //CLR(PORTA,0)
#define stepPin_M2 24 //CLR(PORTA,2)
#define stepPin_M3 44 //CLR(PORTL,5)
#define stepPin_M4 46 //CLR(PORTL,3)
#define dirPin_M1 23 //CLR(PORTA,1)
#define dirPin_M2 25 //CLR(PORTA,3)
#define dirPin_M3 45 //CLR(PORTL,4)
#define dirPin_M4 47 //CLR(PORTL,2)
#define enable1 8 //CLR(PORTH,5)
#define enable2 9 //CLR(PORTH,6)
#define enable3 6 //CLR(PORTH,3)
#define enable4 7 //CLR(PORTH,4)

//Floppy Drives channel_floppy#:
#define floppyStep5_1 26 //CLR(PORTA,4)
#define floppyStep5_2 28 //CLR(PORTA,6)
#define floppyStep5_3 30 //CLR(PORTC,7)
#define floppyDir5_1 27 //CLR(PORTA,5)
#define floppyDir5_2 29 //CLR(PORTA,7)
#define floppyDir5_3 31 //CLR(PORTC,6)

#define floppyStep6_1 32 //CLR(PORTC,5)
#define floppyStep6_2 34 //CLR(PORTC,3)
#define floppyStep6_3 36 //CLR(PORTC,1)
#define floppyDir6_1 33 //CLR(PORTC,4)
#define floppyDir6_2 35 //CLR(PORTC,2)
#define floppyDir6_3 37 //CLR(PORTC,0)

#define floppyStep7_1 38 //CLR(PORTD,7)
#define floppyStep7_2 40 //CLR(PORTG,1)
#define floppyStep7_3 42 //CLR(PORTL,7)
#define floppyDir7_1 39 //CLR(PORTG,2)
#define floppyDir7_2 41 //CLR(PORTG,0)
#define floppyDir7_3 43 //CLR(PORTL,6)

//Hard Drives:
#define bassDrum 2
#define snareDrum 3
#define highDrum 4
#define highDrum2 5

//MIDI Configuration:
#define maxChannel 7 //excluding channel 10
#define stepperChannels 4 //stepper channels
#define floppyChannels 3 //floppy channels
#define hddChannels 4 //hdd channels
#define attack 10000 //initial attack for floppy envelope in us
#define attackOffset 10000 //offset added to initial in us
#define sustain 200000 //initial sustain for floppy envelope in us
#define sustainOffset 50000 //offset subtracted from initial in us

//Variable definitions:
int pitchCurrent[] = { -1, -1, -1, -1, -1, -1, -1, -1};
float bendFactor[] = {1, 1, 1, 1, 1, 1, 1, 1};
byte origPitch[] = { 0, 0, 0, 0, 0, 0, 0, 0};
byte bendSens[] = { 2, 2, 2, 2, 2, 2, 2, 2};
bool motorDirections[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool motorStallMode[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool floppyDir[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prevMicros[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long floppyEnvelope[] = {0, 0, 0, 0, 0, 0, 0, 0}; //prevMicros, but doesnt update each step
unsigned long prevDrum[] = {0, 0, 0, 0, 0}; //bass drum, snare drum, high drum 2, high drum

bool enFloppyDrum = true; //enable floppy drum
unsigned long currentMicros = 0; //master timer
byte controlValue1 = -1;
byte controlValue2 = -1;

byte midiChannel;
byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200); //Allows for serial MIDI communication. Comment this line when using mocaLUFA.
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 22; i < 54; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 6; i < 13; i++) {
    digitalWrite(i, HIGH);
  }
  for (int i = 0; i < 70; i++) {
    SET(PORTA, 4);
    SET(PORTA, 6);
    SET(PORTC, 7);
    SET(PORTC, 5);
    SET(PORTC, 3);
    SET(PORTC, 1);
    SET(PORTD, 7);
    SET(PORTG, 1);
    SET(PORTL, 7);
    CLR(PORTA, 4);
    CLR(PORTA, 6);
    CLR(PORTC, 7);
    CLR(PORTC, 5);
    CLR(PORTC, 3);
    CLR(PORTC, 1);
    CLR(PORTD, 7);
    CLR(PORTG, 1);
    CLR(PORTL, 7);
    delay(5);
  }
  for (int i = 26; i < 54; i++) {
    digitalWrite(i, HIGH);
  }
  for (int i = 0; i < 35; i++) {
    SET(PORTA, 4);
    SET(PORTA, 6);
    SET(PORTC, 7);
    SET(PORTC, 5);
    SET(PORTC, 3);
    SET(PORTC, 1);
    SET(PORTD, 7);
    SET(PORTG, 1);
    SET(PORTL, 7);
    CLR(PORTA, 4);
    CLR(PORTA, 6);
    CLR(PORTC, 7);
    CLR(PORTC, 5);
    CLR(PORTC, 3);
    CLR(PORTC, 1);
    CLR(PORTD, 7);
    CLR(PORTG, 1);
    CLR(PORTL, 7);
    delay(5);
  }
  for (int i = 26; i < 54; i++) {
    digitalWrite(i, HIGH);
  }
  cli();//stop interrupts

  //set timer2 interrupt every 128us
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 7.8khz increments
  OCR2A = 255;// = (16*10^6) / (7812.5*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS11 bit for 8 prescaler
  TCCR2B |= (1 << CS11);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  sei();//allow interrupts
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ISR(TIMER2_COMPA_vect) {
  stepperMotors();
  floppyDrives();
}

void loop() {
  currentMicros = micros();
  readMidi();
  drum();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void drum() {
  if (prevDrum[0] > 0) { //bass drum
    if (currentMicros - prevDrum[0] >= 2000) {
      prevDrum[0] = 0;
      CLR(PORTE, 4);
    }
  }
  if (prevDrum[1] > 0) { //snare drum
    if (currentMicros - prevDrum[1] >= 1250) {
      prevDrum[1] = 0;
      CLR(PORTE, 5);
    }
  }
  if (prevDrum[2] > 0) { //high drum 2
    if (currentMicros - prevDrum[2] >= 1500) {
      prevDrum[2] = 0;
      CLR(PORTE, 3);
    }
  }
  if (prevDrum[3] > 0) { //high drum
    if (currentMicros - prevDrum[3] >= 500) {
      prevDrum[3] = 0;
      CLR(PORTG, 5);
    }
  }
  if (prevDrum[4] > 0) {
    if (currentMicros - prevDrum[4] >= 10000) {
      SET(PORTL, 7);
      SET(PORTG, 1);
      SET(PORTD, 7);
      prevDrum[4] = 0;
    }
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel > 0 && channel <= 10) && velocity > 0)
  {
    if (channel <= 4) //Stepper Motors
    {
      if (channel == 1) {
        CLR(PORTH, 5);
      }
      else if (channel == 2) {
        CLR(PORTH, 6);
      }
      else if (channel == 3) {
        CLR(PORTH, 3);
      }
      else if (channel == 4) {
        CLR(PORTH, 4);
      }
      if (pitch == 50 || pitch == 51 || pitch == 52) {
        motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
      }
      else {
        motorStallMode[channel] = LOW;//No longer stalling itself.
      }
      if (pitch > 81) {
        pitchCurrent[channel] = pitchVals[pitch - 12];
      }
      else {
        pitchCurrent[channel] = pitchVals[pitch];
      }
      origPitch[channel] = pitch;
      prevMicros[channel] = currentMicros;
    }
    else if (channel >= 5 && channel <= 7) { //Floppy Drives
      if (enFloppyDrum && channel == 7) {
        //ignore message
      }
      else {
        pitchCurrent[channel] = pitchVals[pitch];
        floppyEnvelope[channel] = currentMicros;
        prevMicros[channel] = currentMicros;
        origPitch[channel] = pitch;
      }
    }
    else if (channel == 10) {
      if (pitch == 35 || pitch == 36) { //bass drum
        SET(PORTE, 4);
        prevDrum[0] = currentMicros;
      }
      else if (pitch == 38 || pitch == 40) { //snare drum
        SET(PORTE, 5);
        prevDrum[1] = currentMicros;
      }
      else if (pitch == 39 || pitch == 48 || pitch == 50 || pitch == 54 || pitch == 46 || pitch == 51
               || pitch == 59 || pitch == 49 || pitch == 57) { //high drum 2
        SET(PORTE, 3);
        prevDrum[2] = currentMicros;
      }
      else { //high drum
        if (enFloppyDrum) {
          floppyDrum();
        }
        else {
          SET(PORTG, 5);
          prevDrum[3] = currentMicros;
        }
      }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) //MIDI Note OFF Command
{
  if (channel > 0 && channel <= maxChannel)
  {
    if (origPitch[channel] == pitch) {
      pitchCurrent[channel] = -1;//Reset to -1
      origPitch[channel] = 0;
      if (channel == 1)
      {
        SET(PORTH, 5);
      }

      else if (channel == 2)
      {
        SET(PORTH, 6);
      }

      else if (channel == 3)
      {
        SET(PORTH, 3);
      }

      else if (channel == 4)
      {
        SET(PORTH, 4);
      }

      else if (channel == 5) {
        SET(PORTA, 4);
        SET(PORTA, 6);
        SET(PORTC, 7);
      }
      else if (channel == 6) {
        SET(PORTC, 5);
        SET(PORTC, 3);
        SET(PORTC, 1);
      }
      else if (channel == 7) {
        SET(PORTD, 7);
        SET(PORTG, 1);
        SET(PORTL, 7);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void stepperMotors()
{
  if (origPitch[1] > 0 && ((currentMicros - prevMicros[1]) <= pitchCurrent[1]))
  {
    CLR(PORTA, 0);
    SET(PORTA, 0);
    if (motorStallMode[1]) //If we're in stalled motor mode...
    {
      motorDirections[1] = !motorDirections[1]; //Flip the stored direction of the motor.
      if (motorDirections[1]) {
        SET(PORTA, 1);
      }
      else {
        CLR(PORTA, 1);
      }
    }
    prevMicros[1] += pitchCurrent[1] * bendFactor[1]; //Keeps track of the last time a tick occurred for the next time.
  }
  if (origPitch[2] > 0 && ((currentMicros - prevMicros[2]) <= pitchCurrent[2]))
  {
    CLR(PORTA, 2);
    SET(PORTA, 2);
    if (motorStallMode[2]) //If we're in stalled motor mode...
    {
      motorDirections[2] = !motorDirections[2]; //Flip the stored direction of the motor.
      if (motorDirections[2]) {
        SET(PORTA, 3);
      }
      else {
        CLR(PORTA, 3);
      }
    }
    prevMicros[2] += pitchCurrent[2] * bendFactor[2]; //Keeps track of the last time a tick occurred for the next time.
  }
  if (origPitch[3] > 0 && ((currentMicros - prevMicros[3]) <= pitchCurrent[3]))
  {
    CLR(PORTL, 5);
    SET(PORTL, 5);
    if (motorStallMode[3]) //If we're in stalled motor mode...
    {
      motorDirections[3] = !motorDirections[3]; //Flip the stored direction of the motor.
      if (motorDirections[3]) {
        SET(PORTL, 4);
      }
      else {
        CLR(PORTL, 4);
      }
    }
    prevMicros[3] += pitchCurrent[3] * bendFactor[3]; //Keeps track of the last time a tick occurred for the next time.
  }
  if (origPitch[4] > 0 && ((currentMicros - prevMicros[4]) <= pitchCurrent[4]))
  {
    CLR(PORTL, 3);
    SET(PORTL, 3);
    if (motorStallMode[4]) //If we're in stalled motor mode...
    {
      motorDirections[4] = !motorDirections[4]; //Flip the stored direction of the motor.
      if (motorDirections[4]) {
        SET(PORTL, 2);
      }
      else {
        CLR(PORTL, 2);
      }
    }
    prevMicros[4] += pitchCurrent[4] * bendFactor[4]; //Keeps track of the last time a tick occurred for the next time.
  }
}

void floppyDrives() {
  if (origPitch[5] > 0 && ((currentMicros - prevMicros[5]) <= pitchCurrent[5])) {
    step5_1();
    if (currentMicros - floppyEnvelope[5] >= attack && currentMicros - floppyEnvelope[5] <= sustain) {
      step5_2();
      if (currentMicros - floppyEnvelope[5] >= attack + attackOffset && currentMicros - floppyEnvelope[5] <= sustain - sustainOffset) {
        step5_3();
      }
      else {
        SET(PORTC, 7);
      }
    } else {
      SET(PORTA, 6);
    }
    prevMicros[5] += pitchCurrent[5] * bendFactor[5];
  }
  if (origPitch[6] > 0 && ((currentMicros - prevMicros[6]) <= pitchCurrent[6])) {
    step6_1();
    if (currentMicros - floppyEnvelope[6] >= attack && currentMicros - floppyEnvelope[6] <= sustain) {
      step6_2();
      if (currentMicros - floppyEnvelope[6] >= attack + attackOffset && currentMicros - floppyEnvelope[6] <= sustain - sustainOffset) {
        step6_3();
      }
      else {
        SET(PORTC, 1);
      }
    } else {
      SET(PORTC, 3);
    }
    prevMicros[6] += pitchCurrent[6] * bendFactor[6];
  }
  if (origPitch[7] > 0 && ((currentMicros - prevMicros[7]) <= pitchCurrent[7])) {
    step7_1();
    if (currentMicros - floppyEnvelope[7] >= attack && currentMicros - floppyEnvelope[7] <= sustain) {
      step7_2();
      if (currentMicros - floppyEnvelope[7] >= attack + attackOffset && currentMicros - floppyEnvelope[7] <= sustain - sustainOffset) {
        step7_3();
      }
      else {
        SET(PORTL, 7);
      }
    } else {
      SET(PORTG, 1);
    }
    prevMicros[7] += pitchCurrent[7] * bendFactor[7];
  }
}

void step5_1() {
  SET(PORTA, 4);
  CLR(PORTA, 4);
  floppyDir[0] = !floppyDir[0];
  if (floppyDir[0]) {
    SET(PORTA, 5);
  }
  else {
    CLR(PORTA, 5);
  }
}

void step5_2() {
  SET(PORTA, 6);
  CLR(PORTA, 6);
  floppyDir[1] = !floppyDir[1];
  if (floppyDir[1]) {
    SET(PORTA, 7);
  }
  else {
    CLR(PORTA, 7);
  }
}

void step5_3() {
  SET(PORTC, 7);
  CLR(PORTC, 7);
  floppyDir[2] = !floppyDir[2];
  if (floppyDir[2]) {
    SET(PORTC, 6);
  }
  else {
    CLR(PORTC, 6);
  }
}

void step6_1() {
  SET(PORTC, 5);
  CLR(PORTC, 5);
  floppyDir[3] = !floppyDir[3];
  if (floppyDir[3]) {
    SET(PORTC, 4);
  }
  else {
    CLR(PORTC, 4);
  }
}

void step6_2() {
  SET(PORTC, 3);
  CLR(PORTC, 3);
  floppyDir[4] = !floppyDir[4];
  if (floppyDir[4]) {
    SET(PORTC, 2);
  }
  else {
    CLR(PORTC, 2);
  }
}

void step6_3() {
  SET(PORTC, 1);
  CLR(PORTC, 1);
  floppyDir[5] = !floppyDir[5];
  if (floppyDir[5]) {
    SET(PORTC, 0);
  }
  else {
    CLR(PORTC, 0);
  }
}

void step7_1() {
  SET(PORTD, 7);
  CLR(PORTD, 7);
  floppyDir[6] = !floppyDir[6];
  if (floppyDir[6]) {
    SET(PORTG, 2);
  }
  else {
    CLR(PORTG, 2);
  }
}

void step7_2() {
  SET(PORTG, 1);
  CLR(PORTG, 1);
  floppyDir[7] = !floppyDir[7];
  if (floppyDir[7]) {
    SET(PORTG, 0);
  }
  else {
    CLR(PORTG, 0);
  }
}

void step7_3() {
  SET(PORTL, 7);
  CLR(PORTL, 7);
  floppyDir[8] = !floppyDir[8];
  if (floppyDir[8]) {
    SET(PORTL, 6);
  }
  else {
    SET(PORTG, 0);
    SET(PORTG, 0);
  }
}

void floppyDrum() {
  CLR(PORTL, 7);
  CLR(PORTG, 1);
  CLR(PORTD, 7);
  floppyDir[8] = !floppyDir[8];
  if (floppyDir[8]) {
    SET(PORTL, 6);
    SET(PORTG, 2);
    SET(PORTC, 0);
  }
  else {
    CLR(PORTL, 6);
    CLR(PORTG, 2);
    CLR(PORTC, 0);
  }
  prevDrum[4] = currentMicros;
}

void myPitchBend(byte channel, int bend) {
  if (channel > 0 && channel <= maxChannel) {
    float bendF = bend * -1;
    bendF /= 8192;
    bendF *= bendSens[channel];
    bendF /= 12;
    bendFactor[channel] = pow(2, bendF);
  }
}

void myControlChange(byte channel, byte firstByte, byte secondByte) {
  if (channel > 0 && channel <= maxChannel) {
    if (firstByte == 121) { //reset all controllers
      resetAll();
    }
    if (firstByte == 120 || firstByte == 123) { //mute all sounds
      mute();
    }
    if (firstByte == 101) {
      controlValue1 = secondByte;
    }
    if (firstByte == 100) {
      controlValue2 = secondByte;
    }
    if (firstByte == 6 && controlValue1 == 0 && controlValue2 == 0) { //pitch bend sensitivity
      if (secondByte > 0 && secondByte < 13) {
        bendSens[channel] = secondByte;
      }
      controlValue1 = -1;
      controlValue2 = -1;
    }
  }
}

void resetAll() {
  for (int i = 0; i <= maxChannel; i++) {
    bendSens[i] = 2;
    bendFactor[i] = 1;
    pitchCurrent[i] = -1;
  }
  for (int i = 26; i < 54; i += 2) {
    digitalWrite(i, HIGH);
  }
  for (int i = 6; i < 13; i++) {
    digitalWrite(i, HIGH);
  }
}

void mute() {
  for (int i = 0; i <= maxChannel; i++) {
    pitchCurrent[i] = -1;
  }
  for (int i = 26; i < 54; i += 2) {
    digitalWrite(i, HIGH);
  }
  for (int i = 6; i < 13; i++) {
    digitalWrite(i, HIGH);
  }
}

void readMidi() {
  if (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    byte command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      byte midiMsb = (incomingByte & 0b11110000) >> 4; //msb
      midiChannel = (incomingByte & 0b00001111) + 1; //lsb
      if (midiMsb == 9) { //note on
        midiAction = 1;
      }
      else if (midiMsb == 8) { //note off
        midiAction = 3;
      }
      else if (midiMsb == 14) { //pitch bend
        midiAction = 5;
      }
      else if (midiMsb == 11) { //control change
        midiAction = 7;
      }
    }
    else {
      if (midiAction == 1) {
        midiNote = incomingByte;
        midiAction = 2;
      }
      else if (midiAction == 2) {
        handleNoteOn(midiChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 3) {
        midiNote = incomingByte;
        midiAction = 4;
      }
      else if (midiAction == 4) {
        handleNoteOff(midiChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 5) { //receive bendmsb
        midiBendLsb = incomingByte;
        midiAction = 6;
      }
      else if (midiAction == 6) {
        int midiBendVal = (incomingByte << 7) | midiBendLsb;
        myPitchBend(midiChannel, midiBendVal - 8192);
        midiAction = 0;
      }
      else if (midiAction == 7) {
        midiControlNum = incomingByte;
        midiAction = 8;
      }
      else if (midiAction == 8) {
        myControlChange(midiChannel, midiControlNum, incomingByte);
        midiAction = 0;
      }
    }
  }
}
