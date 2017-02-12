/**
   <h1>Pianoduino</h1>
   61 key MIDI Keyboard with Arduino Uno Code, 24 shiftout outputs, 24 shiftin inputs
   1 resistive touchscreen and 16 analog I/O.
   @author Malcolm Davis
   @version 0.1
   @since 2017-02-03
*/

//Arduino Pins
static const int muxAPin = 5;
static const int muxBPin = 4;
static const int muxCPin = 3;
static const int muxReadPin = A0;
static const int inClkPin = 7; //Shift in clock
static const int loadPin = 6; //Shift in or load input. When high data, shifted. When Low data is loaded from parallel inputs.
static const int inEnPin = 8; //Active low
static const int inReadPin = 9;
static const int dataPin = 11;
static const int outClkPin = 12;
static const int latchPin = 10;
//If a Analog mux is added, just connect the control pins in parallel and add the En pin to the list.
static const int muxEn [] = {2};

//Constants
int mask[] = { B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000, B10000000 };//To read the columns values
int blank = B00000000;//To send blank to the register.
byte noteOn = B10010000;
byte noteOff = B10000000;
byte pitchBend = B11100001;
static const short sensitivity = 1000;
static bool debug = false;
static const byte  keyMidi[144] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   B00101000, B00101000, B00101001, B00101001, B00101010, B00101010, B00101011, B00101011, B00101100, B00101100, B00100111, B00100111,
                                   B00101110, B00101110, B00101111, B00101111, B00110000, B00110000, B00101011, B00101011, B00101100, B00101100, B00101101, B00101101,
                                   B00111010, B00111010, B00111011, B00111011, 0, 0, B00110111, B00110111, B00111000, B00111000, B00111001, B00111001,
                                   B00110100, B00110100, B00110101, B00110101, B00110110, B00110110, B00110001, B00110001, B00110010, B00110010, B00110011, B00110011,
                                   B01010010, B01010010, B01010011, B01010011, B01010100, B01010100, B01001111, B01001111, B01010000, B01010000, B01010001, B01010001,
                                   B01001100, B01001100, B01001101, B01001101, B01001110, B01001110, B01001001, B01001001, B01001010, B01001010, B01001011, B01001011,
                                   B01000110, B01000110, B01000111, B01000111, B01001000, B01001000, B01000011, B01000011, B01000100, B01000100, B01000101, B01000101,
                                   B01000000, B01000000, B01000001, B01000001, B01000010, B01000010, B00111101, B00111101, B00111110, B00111110, B00111111, B00111111,
                                   0, 0, 0, 0, B00111100, B00111100, 0, 0, 0, 0, 0, 0
                                  };

//Variables
static bool sValue[] = {0, 0, 0}; // S0, S1, S2 values
long noteTime [72];
bool keyState[144];
bool rowsRead [] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**
   Method to setup the pins and stuffs of the pianoduino.
   @param none
   @return void
*/
void setup() {
  int i;
  Serial.begin(115200);
  pinMode(muxAPin, OUTPUT);
  pinMode(muxBPin, OUTPUT);
  pinMode(muxCPin, OUTPUT);
  pinMode(inClkPin, OUTPUT);
  pinMode(loadPin, OUTPUT);
  pinMode(inEnPin, OUTPUT);
  pinMode(inReadPin, INPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(outClkPin, OUTPUT);
  for (i = 0; i < sizeof(muxEn) / sizeof(int); i++) {
    pinMode(muxEn[i], OUTPUT);
  }
  for (i = 0; i < sizeof(noteTime) / sizeof(long); i++) {
    noteTime[i] = 0;
  }
  for (i = 0; i < sizeof(keyState) / sizeof(bool); i++) {
    keyState[i] = 0;
  }
  digitalWrite(loadPin, HIGH); //On HIGH is read ready
  delay(1000);
}


/**
   Method to select a pin of a mux
   @param pPin: the mux pin that you want to write or read.
   @param pMuxEn: the pin of the mux that you want to read from.
   @return void
*/
void muxToPin(int pPin, int pMuxEn) {
  sValue[0] = bitRead(pPin, 0);
  sValue[1] = bitRead(pPin, 1);
  sValue[2] = bitRead(pPin, 2);
  for (int i = 0; i < sizeof(muxEn) / sizeof(int); i++) {
    digitalWrite(muxEn[i], HIGH);
  }
  digitalWrite(pMuxEn, LOW);
  digitalWrite(muxAPin, sValue[0]);
  digitalWrite(muxBPin, sValue[1]);
  digitalWrite(muxCPin, sValue[2]);
}

/**
   Method to shift the registers to a matrix column
   @param pColumn the column you wish to shift to.
   @return void
*/
void shiftToColumn(int pColumn) {
  digitalWrite(latchPin, LOW);
  if (pColumn < 8) {
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn]);
    shiftOut(dataPin, outClkPin, MSBFIRST, blank);
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn]);
  } else {
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn % 8]);
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn % 8]);
    shiftOut(dataPin, outClkPin, MSBFIRST, blank);
  }
  digitalWrite(latchPin, HIGH);
  delayMicroseconds(5);
}

/**
   Main loop method
   @param none
   @return void
*/
void loop() {
  /**for(int i=0; i<8; i++){
    Serial.print("Y");
    Serial.print(i);
    Serial.print(": ");
    muxToPin(i, muxEn[0]);
    delay(4000);
    Serial.println( (map(analogRead(muxReadPin), 0, 1023, 0, 127)));
    Serial.print(sValue[2]);
    Serial.print(" ");
    Serial.print(sValue[1]);
    Serial.print(" ");
    Serial.println(sValue[0]);
    }*/
  for (int i = 0; i < 16; i++) {
    shiftToColumn(i);
    scanRows();
    for (int j = 0; j < sizeof(rowsRead) / sizeof(bool); j++) {
      if (i < 12 and j < 12) {
        if (rowsRead[j] && !keyState[i + 12 * j]) {
          keyState[i + 12 * j] = 1;
          /*if((i+12*j)%2==0){
            keyState[i+12*j+1]=1;
            }else{
            keyState[i+12*j-1]=1;
            }*/
          if ((i + 12 * j) % 2 == 0) {
            if (keyState[i + 12 * j + 1] == 1) {
              midiSend(noteOn, keyMidi[i + 12 * j], getVelocity((i + 12 * j) / 2));
            } else {
              updateVelocity((i + 12 * j) / 2, (byte)1);
            }
          } else {
            if (keyState[i + 12 * j - 1] == 1) {
              midiSend(noteOn, keyMidi[i + 12 * j], getVelocity((i + 12 * j) / 2));
            }
            else {
              updateVelocity((i + 12 * j) / 2, (byte)1);
            }
          }
          /*Serial.print("Column ");
            Serial.println(i);
            Serial.print("Row ");
            Serial.println(j);*/
        } else {
          if (keyState[i + 12 * j] && !rowsRead[j]) {
            /*keyState[i + 12 * j] = 0;
              midiSend(noteOff, keyMidi[i + 12 * j], 0);*/
            keyState[i + 12 * j] = 0;
            if ((i + 12 * j) % 2 == 0) {
              if (keyState[i + 12 * j + 1] == 0) {
                updateVelocity((i + 12 * j) / 2, (byte)0);
                midiSend(noteOff, keyMidi[i + 12 * j], 0);
              }
            } else {
              if (keyState[i + 12 * j - 1] == 0) {
                updateVelocity((i + 12 * j) / 2, (byte)0);
                midiSend(noteOff, keyMidi[i + 12 * j], 0);
              }
            }
          }
        }
      }
      else {
        //The other buttons..
      }
    }
  }
}

/**
   Method to update the velocity of a note
   @param pPos the position of the note in the array.
   @param pMode 0 to turn off the note, 1 to update.
*/
void updateVelocity(int pPos, byte pMode) {
  if (pMode == 0) {
    noteTime[pPos] = 0;
  } else {
    getVelocity(pPos);
  }
}

/**
   Method that  updates the velocity of a note.
   @param pPos the position of the note in the array.
*/
byte getVelocity(int pPos) {
  if (noteTime[pPos] == 0) {
    noteTime[pPos] = millis();
    /* Serial.println("Time");
      Serial.println(noteTime[pPos], DEC);
      Serial.println("Velocity");
      Serial.println(127, DEC);
      Serial.println("Pos");
      Serial.println(pPos, DEC);
      delay(10);*/
    return 127;
  } else {
    int tempVelocity = map(abs(millis() - noteTime[pPos]), 0, sensitivity, 127, 10);
    noteTime[pPos] = millis();
    if (tempVelocity < 10) {
      tempVelocity = 10;
    }
    else {
      if (tempVelocity > 127) {
        tempVelocity = 127;
      }
    }
    /*Serial.println("Velocity 2");
      Serial.print(tempVelocity, DEC);
      Serial.print(" ");
      Serial.println("Time 2");
      Serial.println(abs(millis()-noteTime[pPos]), DEC);
      Serial.println("Pos 2");
      Serial.println(pPos, DEC);
      delay(10);*/
    return tempVelocity;
  }
}

/**
   Method to scan the values of all the inputs of the shiftregisters
*/
void scanRows() {
  digitalWrite(loadPin, LOW); //On Low the parallel data is collected
  delayMicroseconds(5);
  digitalWrite(loadPin, HIGH); //On HIGH is read ready
  delayMicroseconds(5);
  digitalWrite(inEnPin, LOW); //Active Low
  shiftIn(inReadPin, inClkPin);
  digitalWrite(inEnPin, HIGH); //Active Low
}

/**
   Shift in modification method to read varius shift registers.
   @param pDatapin the pin to read.
   @param pClk the shift registers clock pin.
*/
void shiftIn(int pDatapin, int pClk) {
  for (int i = sizeof(rowsRead) / sizeof(byte) - 1; i >= 0 ; i--)
  {
    digitalWrite(inClkPin, LOW);
    delayMicroseconds(5);
    rowsRead[i] = digitalRead(inReadPin);
    digitalWrite(inClkPin, HIGH);
  }
}


/**
   Method to send the midi messega.
   @param pCmd the midi command code.
   @param pPitch the pitch of the note.
   @param pVelocity of the note.
   @return void
*/
void midiSend(byte pCmd, byte pPitch, byte pVelocity) {
  if (!debug) {
    Serial.write(pCmd);
    Serial.write(pPitch);
    Serial.write(pVelocity);
  } else {
    Serial.println("Begin Message.");
    Serial.println(pCmd, BIN);
    Serial.println(pPitch, BIN);
    Serial.println(pVelocity, BIN);
    Serial.println("End Message.");
  }
}
