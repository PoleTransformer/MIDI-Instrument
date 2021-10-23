byte incomingByte;
byte data;
byte channel;
byte velocity;
byte bendMsb;
byte bendLsb;
byte controlNum;
byte controlVal;
int lsb;
int msb;
byte note = 0;
int action = -1;
int bendVal = 0;

void setup() {
  Serial.begin(115200);
  pinMode(9, OUTPUT);
  cli();//disable interrupts
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  OCR1A = 1;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();//enable interrupts
}

ISR(TIMER1_COMPA_vect) {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    int command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      msb = (incomingByte & 0b11110000) >> 4;
      lsb = incomingByte & 0b00001111;
    }
    else if (command == 0) { //data
      data = incomingByte;
    }
    if (msb == 9 && action == -1) { //note on
      channel = lsb;
      action = 0;
    }
    else if (action == 0) {
      note = data;
      action = 1;
    }
    else if (action == 1) {
      velocity = data;
      action = -1;
    }
    if (msb == 8) { //note off
      channel = lsb;
      note = 0;
    }
    if (msb == 11 && action == -1) { //control change
      channel = lsb;
      action = 2;
    }
    else if (action == 2) { //receive controller value
      controlNum = data;
      action = 3;
    }
    else if (action == 3) {
      controlVal = data;
      action = -1;
    }
    if (msb == 14 && action == -1) { //pitch bend
      channel = lsb;
      action = 4;
    }
    else if (action == 4) { //receive bendmsb
      bendLsb = data;
      action = 5;
    }
    else if (action == 5) {
      bendMsb = data;
      bendVal = (bendMsb << 7) | bendLsb;
      bendVal *= -1;
      action = -1;
    }
  }
}

void loop () {
  if (note != 0) {
    tone(9, 440 * pow(2, ((float)note - 69) / 12 + ((float)bendVal - 8192) / (4096 * 12)));
  }
  else if(note == 0) {
    noTone(9);
  }
}
