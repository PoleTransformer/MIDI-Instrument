byte incomingByte;
byte midiData;
byte midiChannel;
byte midiVelocity;
byte midiBendMsb;
byte midiBendLsb;
int midiLsb;
int midiMsb;
byte midiNote = 0;
int midiBendVal = 0;
int midiAction = -1;

/////////////////////////

float bendFactor = 1;
int frequency = -1;
byte origPitch = 0;

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  pinMode(9, OUTPUT);
  cli();//disable interrupts
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  OCR1A = 19;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 for 8 prescaler
  TCCR1B |= (1 << CS11);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();//enable interrupts
}

ISR(TIMER1_COMPA_vect) {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    int command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      midiMsb = (incomingByte & 0b11110000) >> 4;
      midiLsb = incomingByte & 0b00001111;
      midiChannel = midiLsb;
    }
    else if (command == 0) { //data
      midiData = incomingByte;
    }
    if (midiMsb == 9 && midiAction == -1) { //note on
      midiAction = 0;
    }
    else if (midiAction == 0) {
      midiNote = midiData;
      midiAction = 1;
    }
    else if (midiAction == 1) {
      midiVelocity = midiData;
      handleNoteOn(midiChannel+1, midiNote, midiVelocity);
      midiAction = -1;
    }
    if (midiMsb == 8 && midiAction == -1) { //note off
      midiAction = 2;
    }
    else if (midiAction == 2) {
      midiNote = midiData;
      midiAction = 3;
    }
    else if (midiAction == 3) {
      midiVelocity = midiData;
      handleNoteOff(midiChannel+1, midiNote, midiVelocity);
      midiAction = -1;
    }
    if (midiMsb == 14 && midiAction == -1) { //pitch bend
      midiAction = 4;
    }
    else if (midiAction == 4) { //receive bendmsb
      midiBendLsb = midiData;
      midiAction = 5;
    }
    else if (midiAction == 5) {
      midiBendMsb = midiData;
      midiBendVal = (midiBendMsb << 7) | midiBendLsb;
      myPitchBend(midiChannel+1, midiBendVal - 8192);
      midiAction = -1;
    }
  }
}

void loop () {
  if(frequency != -1) {
    tone(9,frequency*bendFactor);
  }
  else {
    noTone(9);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if(channel == 1 && velocity > 0) {
    origPitch = note;
    frequency = (440 * pow(2, ((float)note - 69) / 12));
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if(channel == 1) {
    if(note == origPitch) {
      frequency = -1;
    }
  }
}

void myPitchBend(byte channel, int bend) {
  float bendF = bend;
  bendF /= 8192;
  bendF *= 12;
  bendF /= 12;
  bendFactor = pow(2, bendF);
}
