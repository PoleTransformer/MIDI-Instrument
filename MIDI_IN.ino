byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

void setup() {
  Serial.begin(115200);
}

void loop() {
  readMIDI();
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  
}

void handlePitchBend(byte channel, int bend) {
  
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  
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
