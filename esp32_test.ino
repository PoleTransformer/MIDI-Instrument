byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;
bool resetFlag;

#define floppyChannels 2
#define threshold1 64 //Sustain threshold 0-127
#define threshold2 90
#define threshold3 120
unsigned int floppyData[] = {17, 15}; //data, latch, clock
unsigned int floppyLatch[] = {5, 4}; //data, latch, clock
unsigned int floppyClock[] = {18, 16}; //data, latch, clock
byte floppyState[17];
byte volume[17];
byte sustainVal[17];
bool floppyDir[17];
unsigned long previousFloppy[17];
unsigned int attackVal[17];
unsigned int decayVal[17];

#define stepperChannels 4
unsigned int stepper[] = {21, 19, 32, 33};

unsigned long currentTime;
unsigned long previousTime[17];
unsigned int currentPeriod[17];
byte currentNote[17];
byte bendSens[17];
byte controlVal1[17];
byte controlVal2[17];
float bendFactor[17];
float originalBend[17];
float maxBend[17];
bool vibratoState[17];
bool trill[17];

#define drumCount 3
unsigned int drums[] = {22, 23, 2};
unsigned long previousDrum[17];
unsigned int drumDuration[17];

hw_timer_t * timer = NULL;
hw_timer_t * timer2 = NULL;
volatile bool interruptbool1 = false;
volatile bool interruptbool2 = false;

const unsigned int pitchVals[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  61162, 57737, 54496, 51414, 48544, 45809, 43253, 40816, 38521, 36364, 34317, 32394, //C0 - B0
  30581, 28860, 27241, 25714, 24272, 22910, 21622, 20408, 19264, 18182, 17161, 16197, //C1 - B1
  15288, 14430, 13620, 12857, 12134, 11453, 10811, 10204, 9631, 9091, 8581, 8099, //C2 - B2
  7645, 7216, 6811, 6428, 6068, 5727, 5405, 5102, 4816, 4545, 4290, 4050, //C3 - B3
  3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551, 2408, 2273, 2145, 2025, //C4 - B4
  1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012, //C5 - B5
  956, 902, 851, 804, 758, 716, 676, 638, 602, 568, 536, 506, //C6 - B6
  478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253, //C7 - B7
  239, 225, 213, 201, 190, 179, 169, 159, 150, 142, 134, 127,//C8 - B8
  0, 0, 0, 0, 0, 0, 0, 0
};

void IRAM_ATTR onTimer() {
  interruptbool1 = true;
}

void IRAM_ATTR onTimer2() {
  interruptbool2 = true;
}

void myShiftOut(uint8_t val, byte channel) {
  uint8_t i;
  digitalWrite(floppyLatch[channel - 5], LOW); //latch pin low
  for (int i = 0; i < 8; i++) {
    if (val & (1 << (7 - i))) {
      digitalWrite(floppyData[channel - 5], HIGH); //data high
    }
    else {
      digitalWrite(floppyData[channel - 5], LOW); //data low
    }
    digitalWrite(floppyClock[channel - 5], HIGH); //clock pin high
    digitalWrite(floppyClock[channel - 5], LOW); //clock pin low
  }
  digitalWrite(floppyLatch[channel - 5], HIGH); //latch pin high
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < stepperChannels; i++) {
    pinMode(stepper[i], OUTPUT);
  }
  for (int i = 0; i < floppyChannels; i++) {
    pinMode(floppyData[i], OUTPUT);
    pinMode(floppyLatch[i], OUTPUT);
    pinMode(floppyClock[i], OUTPUT);
  }
  for (int i = 0; i < drumCount; i++) {
    pinMode(drums[i], OUTPUT);
  }
  reset();
  timer = timerBegin(0, 80, true);
  timer2 = timerBegin(1, 40, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  timerAlarmWrite(timer, 80000, true);
  timerAlarmWrite(timer2, 80000, true);
  timerAlarmEnable(timer);
  timerAlarmEnable(timer2);
}

void loop() {
  readMIDI();
  currentTime = micros();
  for (int i = 1; i <= stepperChannels; i++) {
    if (currentNote[i] > 0 && currentTime - previousTime[i] >= currentPeriod[i]) {
      previousTime[i] = currentTime;
      digitalWrite(stepper[i - 1], HIGH);
      digitalWrite(stepper[i - 1], LOW);
    }
    if (drumDuration[i] > 0 && currentTime - previousDrum[i] >= 1000) {
      previousDrum[i] = currentTime;
      drumDuration[i]--;
      digitalWrite(stepper[i - 1], HIGH);
      digitalWrite(stepper[i - 1], LOW);
    }
  }
  for (int i = 5; i < 5 + floppyChannels; i++) {
    if (currentNote[i] > 0 && currentTime - previousTime[i] >= currentPeriod[i]) {
      previousTime[i] = currentTime;
      myShiftOut(floppyState[i] | B01010101, i);
      myShiftOut(floppyState[i], i);
      ADSR(i);
      floppyDir[i] = !floppyDir[i];
      if (floppyDir[i]) {
        floppyState[i] |= B10101010;
        myShiftOut(floppyState[i], i);
      }
      else {
        floppyState[i] &= B01010101;
        myShiftOut(floppyState[i], i);
      }
    }
  }
  for (int i = 5; i < 5 + drumCount; i++) {
    if (drumDuration[i] > 0 && currentTime - previousDrum[i] >= drumDuration[i]) {
      previousDrum[i] = currentTime;
      drumDuration[i] = 0;
      digitalWrite(drums[i - 5], LOW);
    }
  }
  if (interruptbool1) {
    interruptbool1 = false;
    for (int i = 1; i < 17; i++) {
      if (maxBend[i] > 0) {
        if (vibratoState[i]) {
          bendFactor[i] = originalBend[i] + maxBend[i];
        }
        else {
          bendFactor[i] = originalBend[i] - maxBend[i];
        }
        vibratoState[i] = !vibratoState[i];
        currentPeriod[i] = pitchVals[currentNote[i]] * bendFactor[i];
      }
    }
  }
  if (interruptbool2) {
    interruptbool2 = false;
    for (int i = 1; i < 17; i++) {
      if (trill[i]) {
        if (vibratoState[i]) {
          currentPeriod[i] = pitchVals[currentNote[i] + 12] * bendFactor[i];
        }
        else {
          currentPeriod[i] = pitchVals[currentNote[i]] * bendFactor[i];
        }
        vibratoState[i] = !vibratoState[i];
      }
    }
  }
}

void ADSR(byte channel) {
  //attack
  if (currentTime - previousFloppy[channel] >= attackVal[channel] && volume[channel] >= threshold1) {
    floppyState[channel] &= B11111011;
  }
  if (currentTime - previousFloppy[channel] >= attackVal[channel] * 2 && volume[channel] >= threshold2) {
    floppyState[channel] &= B11101111;
  }
  if (currentTime - previousFloppy[channel] >= attackVal[channel] * 3 && volume[channel] >= threshold3) {
    floppyState[channel] &= B10111111;
  }
  //decay
  if (currentTime - previousFloppy[channel] >= (attackVal[channel] * 3) + decayVal[channel] && sustainVal[channel] <= 96) {
    floppyState[channel] |= B01000000;
  }
  if (currentTime - previousFloppy[channel] >= (attackVal[channel] * 3) + (decayVal[channel] * 2) && sustainVal[channel] <= 64) {
    floppyState[channel] |= B00010000;
  }
  if (currentTime - previousFloppy[channel] >= (attackVal[channel] * 3) + (decayVal[channel] * 3) && sustainVal[channel] <= 32) {
    floppyState[channel] |= B00000100;
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  }
  else if (channel == 10) {
    if (note == 35 || note == 36) {
      drumDuration[1] = map(velocity, 1, 127, 1, 5);
      drumDuration[5] = map(velocity, 1, 127, 1, 2400); //bass hdd
      previousDrum[5] = currentTime;
      digitalWrite(drums[0], HIGH);
    }
    else if (note == 38 || note == 40) {
      drumDuration[2] = map(velocity, 1, 127, 1, 10);
      drumDuration[6] = map(velocity, 1, 127, 1, 2050); //snare hdd
      previousDrum[6] = currentTime;
      digitalWrite(drums[1], HIGH);
    }
    else {
      drumDuration[3] = map(velocity, 1, 127, 1, 3);
      if (note == 39 || note == 46 || note == 49 || note == 50 || note == 51 || note == 54) {
        drumDuration[7] = map(velocity, 1, 127, 5500, 6500);
        previousDrum[7] = currentTime;
        digitalWrite(drums[2], HIGH); //solenoid
      }
    }
  }
  else {
    if (channel >= 5 && channel < 5 + floppyChannels) {
      previousFloppy[channel] = currentTime;
      floppyState[channel] |= B01010100;
      volume[channel] = velocity;
      resetFlag = true;
    }
    currentNote[channel] = note;
    currentPeriod[channel] = pitchVals[note] * bendFactor[channel];
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == currentNote[channel]) {
    currentNote[channel] = 0;
    if (channel >= 5 && channel < 5 + floppyChannels) {
      myShiftOut(floppyState[channel] | B01010101, channel);
    }
  }
}

void handlePitchBend(byte channel, int bend) {
  float bendF = bend;
  bendF /= 8192;
  bendF *= bendSens[channel];
  bendF /= 12;
  bendFactor[channel] = pow(2, bendF);
  originalBend[channel] = bendFactor[channel];
  currentPeriod[channel] = pitchVals[currentNote[channel]] * bendFactor[channel];
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) { //Modulation
    if (secondByte > 0) {
      maxBend[channel] = myMap(secondByte, 1, 127, 0, 0.03);
    }
    else {
      maxBend[channel] = 0;
      bendFactor[channel] = originalBend[channel];
      currentPeriod[channel] = pitchVals[currentNote[channel]] * bendFactor[channel];
    }
  }
  if (firstByte == 12) { //floppy sustain
    sustainVal[channel] = secondByte;
  }
  if (firstByte == 13) { //decay
    decayVal[channel] = map(secondByte, 0, 127, 0, 100000);
  }
  if (firstByte == 14) { //stepper special effect
    if (secondByte > 0) {
      trill[channel] = true;
    }
    else {
      trill[channel] = false;
      currentPeriod[channel] = pitchVals[currentNote[channel]] * bendFactor[channel];
    }
  }
  if (firstByte == 15) { //attack
    attackVal[channel] = map(secondByte, 0, 127, 0, 100000);
  }
  if (firstByte == 100) {
    controlVal1[channel] = secondByte;
  }
  if (firstByte == 101) {
    controlVal2[channel] = secondByte;
  }
  if (firstByte == 6 && controlVal1[channel] == 0 && controlVal2[channel] == 0) {
    bendSens[channel] = secondByte;
    controlVal1[channel] = 127;
    controlVal2[channel] = 127;
  }
  if (firstByte == 121 && resetFlag) {
    resetFlag = false;
    reset();
  }
}

float myMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void reset() {
  for (int i = 0; i < 17; i++) {
    bendFactor[i] = 1;
    originalBend[i] = 1;
    bendSens[i] = 2;
    maxBend[i] = 0;
    currentNote[i] = 0;
    attackVal[i] = 10000;
    decayVal[i] = 30000;
    sustainVal[i] = 0;
    drumDuration[i] = 0;
  }
  for (int i = 0; i < drumCount; i++) {
    digitalWrite(drums[i], LOW);
  }
  for (int i = 5; i < 5 + floppyChannels; i++) {
    for (int j = 0; j < 160; j++) {
      myShiftOut(B00000000, i);
      myShiftOut(B01010101, i);
      delay(5);
    }
    for (int k = 0; k < 40; k++) {
      myShiftOut(B10101010, i);
      myShiftOut(B11111111, i);
      delay(5);
    }
  }
}

void readMIDI() {
  if (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    byte command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      byte midiMsb = (incomingByte & 0b11110000) >> 4;
      offsetChannel = (incomingByte & 0b00001111) + 1;
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
      else if (midiAction == 2) { //velocity
        handleNoteOn(offsetChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 3) {
        midiNote = incomingByte;
        midiAction = 4;
      }
      else if (midiAction == 4) {
        handleNoteOff(offsetChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 5) { //receive bendmsb
        midiBendLsb = incomingByte;
        midiAction = 6;
      }
      else if (midiAction == 6) {
        int midiBendVal = (incomingByte << 7) | midiBendLsb;
        handlePitchBend(offsetChannel, (midiBendVal - 8192) * -1);
        midiAction = 0;
      }
      else if (midiAction == 7) {
        midiControlNum = incomingByte;
        midiAction = 8;
      }
      else if (midiAction == 8) {
        handleControlChange(offsetChannel, midiControlNum, incomingByte);
        midiAction = 0;
      }
    }
  }
}
