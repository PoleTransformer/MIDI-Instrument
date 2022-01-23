byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

unsigned long currentMicros;
unsigned long previousMicros[17];
unsigned long currentPeriod[17];

bool floppyDir[17];
bool enableVibrato[17];
bool vibratoState[17];
float bendFactor[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
float originalBend[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
byte bendSens[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
byte currentNote[17];
byte controlVal1 = 127;
byte controlVal2 = 127;
byte pinConfiguration[] = {0, 4, 16, 17, 5}; //step pin followed by dir pin
int pinSize = sizeof(pinConfiguration);
int availableFloppy = (pinSize - 1) / 2;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const unsigned long notePeriods[128] = {
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
  for (int i = 1; i <= availableFloppy; i++) {
    if (enableVibrato[i]) {
      if (vibratoState[i]) {
        bendFactor[i] = originalBend[i] + 0.02;
      }
      else {
        bendFactor[i] = originalBend[i] - 0.02;
      }
      vibratoState[i] = !vibratoState[i];
      currentPeriod[i] = notePeriods[currentNote[i]] * bendFactor[i];
    }
  }
}

void setup() {
  for (int i = 1; i < pinSize; i++) {
    pinMode(pinConfiguration[i], OUTPUT);
  }
  for (int i = 1; i < 160; i++) {
    for (int i = 1; i < pinSize; i += 2) {
      digitalWrite(pinConfiguration[i], HIGH);
      digitalWrite(pinConfiguration[i], LOW);
    }
    delay(5);
  }
  for (int i = 2; i < pinSize; i += 2) {
    digitalWrite(pinConfiguration[i], HIGH);
  }
  for (int i = 1; i < 40; i++) {
    for (int i = 1; i < pinSize; i += 2) {
      digitalWrite(pinConfiguration[i], HIGH);
      digitalWrite(pinConfiguration[i], LOW);
    }
    delay(5);
  }
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 80000, true);
  timerAlarmEnable(timer);
  Serial.begin(115200);
}

void loop() {
  currentMicros = micros();
  readMIDI();
  for (int i = 1; i <= availableFloppy; i++) {
    if (currentPeriod[i] > 0 && currentMicros - previousMicros[i] >= currentPeriod[i]) {
      previousMicros[i] = currentMicros;
      floppyDir[i] = !floppyDir[i];
      int pin = i + (i - 1);
      digitalWrite(pinConfiguration[pin], HIGH);
      digitalWrite(pinConfiguration[pin], LOW);
      digitalWrite(pinConfiguration[pin + 1], floppyDir[i]);
    }
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  }
  else {
    currentNote[channel] = note;
    currentPeriod[channel] = notePeriods[note] * bendFactor[channel];
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == currentNote[channel]) {
    currentPeriod[channel] = 0;
    currentNote[channel] = 0;
  }
}

void handlePitchBend(byte channel, int bend) {
  float bendF = bend;
  bendF /= 8192;
  bendF *= bendSens[channel];
  bendF /= 12;
  bendFactor[channel] = pow(2, bendF);
  originalBend[channel] = bendFactor[channel];
  currentPeriod[channel] = notePeriods[currentNote[channel]] * bendFactor[channel];
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) {
    if (secondByte > 0) {
      enableVibrato[channel] = true;
    }
    else {
      enableVibrato[channel] = false;
      bendFactor[channel] = originalBend[channel];
      currentPeriod[channel] = notePeriods[currentNote[channel]] * bendFactor[channel];
    }
  }
  if (firstByte == 100) {
    controlVal1 = secondByte;
  }
  if (firstByte == 101) {
    controlVal2 = secondByte;
  }
  if (firstByte == 6 && controlVal1 == 0 && controlVal2 == 0) {
    bendSens[channel] = secondByte;
    controlVal1 = 127;
    controlVal2 = 127;
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
