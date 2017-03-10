/**
 * <h1>Pianoduino</h1>
 * 61 key MIDI Keyboard with Arduino Uno Code, 24 shiftout outputs, 24 shiftin inputs
 * 1 resistive touchscreen and 16 analog I/O.
 * @author Malcolm Davis
 * @version 0.1
 * @since 2017-02-03
 */

//*********************Arduino Pins*************************//
static const byte muxAPin = 2;
static const byte muxBPin = 3;
static const byte muxCPin = 4;
static const byte muxReadPin = A0;
static const byte inClkPin = 7; //Shift in clock
static const byte loadPin = 6; //Shift in or load input. When high data, shifted. When Low data is loaded from parallel inputs.
static const byte inEnPin = 8; //Active low
static const byte inReadPin = 9;
static const byte dataPin = 11;
static const byte outClkPin = 12;
static const byte latchPin = 10;
//If a Analog mux is added, just connect the control pins in parallel and add the En pin to the list.
static const byte muxEn [] = {
  5};
//**********************************************************//

//**************************Constants*********************//
byte mask[] = { 
  B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000, B10000000 };//To read the columns values
static const byte  keyMidiCons[144] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
static const byte drumNotes []= {
  B00100110
}; //Drums note, snaredrum,...
static const byte blank = B00000000;//To send blank to the register.
static const byte noteON = B10010000;
static const byte noteOFF = B10000000;
static const byte control = B10110000;
static const byte pitchBend = B11100000;
static const byte  padMidi = B00000000; //Pad first note.
static const byte  buttonMidi = B00010000; //Controll general first code.
static const byte analogMidi =B00110000; //Controll general first code.
// MIDI codes taken from http://nickfever.com/music/midi-cc-list
static const byte MIDI_CC_MODULATION = B00000001;
static const byte MIDI_CC_VOLUME = B00000111;
static const byte MIDI_CC_BALANCE = B00001000;
static const byte MIDI_CC_SUSTAIN = B00101000;
static const byte MIDI_CC_NOTES_OFF = B01111011;



//Shift in movements(3 shift registers and 8 pins per shift register)
//The sum of all the rows have to be smaller or equal to the shift in registers.
static const byte shiftIns=24; 
//*******************Configuration constants**********************//
//Don't change this values unless you change the hardware.
static const byte keyColumns = 12;
static const byte keyRows = 12; //
static const byte buttonColumns = 3;
static const byte buttonRows = 4; 
static const byte padColumns = 4;
static const byte padRows = 4;
//This values can be changed.
static const short sensitivity = 1000;
static const byte keyboardChannel = 1;
static const byte buttonPadChannel = 2;
static const byte controlChannel = 3;
static const byte drumsChannel = 4;
static const byte drumsThreshold [] = { 
40 
};//Drums threshold, snaredrum,...
static const byte analogThreshold = 1;
//#define debug //Uncomment to debug
//***************************************************************//

//*******************************Variables**********************//
static bool sValue[] = {
  0, 0, 0}; // S0, S1, S2 values
long noteTime [sizeof(keyMidiCons) / sizeof(byte) / 2]; //Time that the note have been pressed.
bool keyState[sizeof(keyMidiCons) / sizeof(byte)]; //State of the key.
bool buttonState[buttonColumns*buttonRows]; //State of the buttons.
bool padState[padRows*padColumns]; //State of the pad buttons.
bool rowsRead [keyRows+buttonRows]; // Rows shift in values.
byte trans = 0; //Level to transpose the octaves.
static byte  keyMidi[sizeof(keyMidiCons) / sizeof(byte)]; 
byte analogValue[(sizeof(muxEn) / sizeof(byte)) * 8]; 
//**************************************************************//


/**
 * Method to setup the pins and stuffs of the pianoduino.
 * @param none
 * @return void
 */
void setup() {
  byte i;
  Serial.begin(115200);
#ifdef debug 
  Serial.println("Setup Start");
#endif
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
  //Set the enabler pins to outputs.
  for (i = 0; i < sizeof(muxEn) / sizeof(int); i++) {
    pinMode(muxEn[i], OUTPUT);
  }
  //Set the note times to 0
  for (i = 0; i < sizeof(noteTime) / sizeof(long); i++) {
    noteTime[i] = 0;
  }
  //Set all the notes as off
  for (i = 0; i < sizeof(keyState) / sizeof(bool); i++) {
    keyState[i] = 0;
  }
  //Set all the row read values to false
  for (i = 0; i < keyRows+buttonRows; i++){
    rowsRead[i] = false;
  }
  //Set all the buttons of the pad to off.
  for (i = 0; i < padColumns*padRows; i++){
    padState[i] = 0;
  }
  //Set all the buttons of the pad to off.
  for (i = 0; i < buttonRows*buttonColumns; i++){
    buttonState[i] = 0;
  }

  readAnalog(); //Read the initial value of the knobs
  transpose(trans); //Set the octave to the initial value.
  digitalWrite(loadPin, HIGH); //On HIGH is read ready
  delay(1000); //Waits a second.
#ifdef debug 
  Serial.println("Setup Ready");
#endif
}

/**
 * Method to select a pin of a mux
 * @param pPin: the mux pin that you want to write or read.
 * @param pMuxEn: the pin of the mux that you want to read from.
 * @return void
 */
void muxToPin(byte pPin, byte pMuxEn) {
  sValue[0] = bitRead(pPin, 0);
  sValue[1] = bitRead(pPin, 1);
  sValue[2] = bitRead(pPin, 2);
  for (byte i = 0; i < sizeof(muxEn) / sizeof(byte); i++) {
    if (pMuxEn != i) {
      digitalWrite(muxEn[i], HIGH);
    }
  }
  digitalWrite(pMuxEn, LOW);
  digitalWrite(muxAPin, sValue[0]);
  digitalWrite(muxBPin, sValue[1]);
  digitalWrite(muxCPin, sValue[2]);
}

/**
 * Method to shift the registers to a matrix column
 * @param pColumn the column you wish to shift to.
 * @return void
 */
void shiftToColumn(byte pColumn) {
#ifdef debug 
  Serial.print("Shifting to column: ");
  Serial.println(pColumn);
  delay(10);
#endif
  digitalWrite(latchPin, LOW);
  if (pColumn < 8) {
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn]);
    shiftOut(dataPin, outClkPin, MSBFIRST, blank);
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn]);
  } 
  else {
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn % 8]);
    shiftOut(dataPin, outClkPin, MSBFIRST, mask[pColumn % 8]);
    shiftOut(dataPin, outClkPin, MSBFIRST, blank);
  }
  digitalWrite(latchPin, HIGH);
#ifdef debug 
  Serial.println("Shift done.");
#endif
  delayMicroseconds(5);
}

/**
 * Method to transpose the notes
 * @param pDir direction to transpose
 */
void transpose(byte pDir) {
  if (-3 < trans + pDir ) {
    if (trans + pDir < 5) {
      trans = trans + pDir;
    } 
    else {
      trans = 4;
    }
  } 
  else {
    trans = -2;
  }
  for (byte i = 0; i <  sizeof(keyMidi) / sizeof(byte); i++) {
    keyMidi[i] = keyMidiCons[i] + trans * 12;
  }
}

/**
 * Method to read all the values of the knobs and other analog components. (Like the snaredrum.
 * Updates the analogValue array.
 */
void readAnalog() {
#ifdef debug 
  Serial.println("readAnalog start");
#endif
  byte tmp = 0;
  for (byte i = 0; i < sizeof(muxEn) / sizeof(byte); i++) {
    for (byte y = 0; y < 8; y++) {
      muxToPin(y, muxEn[i]);
      tmp = map(analogRead(muxReadPin), 0, 1023, 0, 127);
      if (i * 8 + y < 15) {
        //If the value is greater or smaller than the actual value plus or minus the threshold send the change value.
        if (tmp > analogValue[i * 8 + y] + analogThreshold || tmp < analogValue[i * 8 + y] - analogThreshold  ) {

          analogValue[i * 8 + y] = tmp;
          //Send message with the knoobs value
          controlMessage(controlChannel, i * 8 + y, tmp);
        }
      }
      else if(tmp > drumsThreshold[(i * 8 + y)%15]){
        //Send message with the drums value
        noteOn(drumsChannel, drumNotes[(i * 8 + y)%15], tmp);
      }
    }
  }
#ifdef debug 
  Serial.println("readAnalog pass");
#endif
}

/**
 * Method to read all the values of the keys and send the notes through MIDI.
 */
void readKeys(){
  for (byte i = 0; i < keyColumns; i++) {
    shiftToColumn(i);
    scanRows();
    for (byte j = 0; j < keyRows; j++) {
      if (rowsRead[j] && !keyState[i + keyColumns * j]) {
        keyState[i + keyColumns * j] = 1;
        if ((i + keyColumns * j) % 2 == 0) {
          if (keyState[i + keyColumns * j + 1] == 1) {
            noteOn(keyboardChannel, keyMidi[i + keyColumns * j], getVelocity((i + keyColumns * j) / 2));
          } 
          else {
            updateVelocity((i + keyColumns * j) / 2, (byte)1);
          }
        } 
        else {
          if (keyState[i + 12 * j - 1] == 1) {
            noteOn(keyboardChannel, keyMidi[i + keyColumns * j], getVelocity((i + keyColumns * j) / 2));
          }
          else {
            updateVelocity((i + keyColumns * j) / 2, (byte)1);
          }
        }
#ifdef debug
        Serial.print("Column ");
        Serial.println(i);
        Serial.print("Row ");
        Serial.println(j);
#endif
      } 
      else {
        if (keyState[i + keyColumns * j] && !rowsRead[j]) {
          keyState[i + 12 * j] = 0;
          if ((i + 12 * j) % 2 == 0) {
            if (keyState[i + 12 * j + 1] == 0) {
              updateVelocity((i + 12 * j) / 2, (byte)0);
              noteOff(keyboardChannel, keyMidi[i + 12 * j]);
            }
          } 
          else {
            if (keyState[i + 12 * j - 1] == 0) {
              updateVelocity((i + 12 * j) / 2, (byte)0);
              noteOff(keyboardChannel, keyMidi[i + 12 * j]);
            }
          }
        }
      }
    }
  }
}

/**
 * Method to read all the values of the buttons and send the notes through MIDI.
 */
void readButtons(){
  byte buttonIndex = 0;
  for (byte i = keyColumns; i < buttonColumns+keyColumns; i++) {//Is done like this for the shift register index.
    shiftToColumn(i);
    scanRows();
    for (byte j = keyRows; j < keyRows+buttonRows; j++) {//Is done like this for the shift register index
      if (rowsRead[j] && !buttonState[buttonIndex]) {
        buttonState[buttonIndex] = 1;
        controlMessage(controlChannel, buttonMidi + buttonIndex, 127); 
#ifdef debug
        Serial.print("Column ");
        Serial.println(i);
        Serial.print("Row ");
        Serial.println(j);
#endif
      } 
      else {
        if (buttonState[buttonIndex] && !rowsRead[j]) {
          buttonState[buttonIndex] = 0;
        }
      }
      buttonIndex++;
    }
  }
}

/**
 * Method to update the velocity of a note
 * @param pPos the position of the note in the array.
 * @param pMode 0 to turn off the note, 1 to update.
 */
void updateVelocity(byte pPos, byte pMode) {
  if (pMode == 0) {
    noteTime[pPos] = 0;
  } 
  else {
    getVelocity(pPos);
  }
}

/**
 * Method that  updates the velocity of a note.
 * @param pPos the position of the note in the array.
 */
byte getVelocity(byte pPos) {
  if (noteTime[pPos] == 0) {
    noteTime[pPos] = millis();
    return 127;
  } 
  else {
    byte tempVelocity = map(abs(millis() - noteTime[pPos]), 0, sensitivity, 127, 10);
    noteTime[pPos] = millis();
    if (tempVelocity < 10) {
      tempVelocity = 10;
    }
    else {
      if (tempVelocity > 127) {
        tempVelocity = 127;
      }
    }
    return tempVelocity;
  }
}

/**
 * Method to scan the values of all the inputs of the shiftregisters
 */
void scanRows() {
#ifdef debug 
  Serial.println("Scanning rows start");
  Serial.print("Load pin: ");
  Serial.println(loadPin);
  Serial.print("inEnPin pin: ");
  Serial.println(inEnPin);
  Serial.print("inReadPin pin: ");
  Serial.println(inReadPin);
  delay(10);
#endif
  digitalWrite(loadPin, LOW); //On Low the parallel data is collected
  delayMicroseconds(5);
  digitalWrite(loadPin, HIGH); //On HIGH is read ready
  delayMicroseconds(5);
  digitalWrite(inEnPin, LOW); //Active Low
  shiftIn(inReadPin, inClkPin);
  digitalWrite(inEnPin, HIGH); //Active Low
#ifdef debug 
  Serial.println("Scanning rows end");
  delay(1);
#endif
}

/**
 * Shift in modification method to read varius shift registers.
 * @param pDatapin the pin to read.
 * @param pClk the shift registers clock pin.
 */
void shiftIn(byte pDatapin, byte pClk) {
#ifdef debug 
  Serial.println("ShiftIn Start");
  delay(1);
#endif
  for (short i = shiftIns - 1; i >= 0 ; i--)
  {
    digitalWrite(inClkPin, LOW);
    delayMicroseconds(5);
    rowsRead[i] = digitalRead(inReadPin);
    digitalWrite(inClkPin, HIGH);
  }
#ifdef debug
  Serial.println("ShiftIn End");
  delay(10);
#endif
}

/**
 * Method to send a control message with analog values.
 * @param pChannel the channel to send the message.
 * @param pControl the note to turn on.
 * @param pValue the value to send.
 */
void controlMessage(byte pChannel, byte pControl, byte pValue){
  pChannel += control-1;
  midiSend(pChannel,pControl, pValue);
}

/**
 * Method to send a note on message.  
 * @param pChannel the channel to send the noteon.
 * @param pPitch the note to turn on.
 * @param pVelocity the velocity of the note.
 */
void noteOn(byte pChannel, byte pPitch, byte pVelocity){
  pChannel += noteON-1;
  midiSend(pChannel, pPitch, pVelocity);
}

/**
 * Method to send a note off message.
 * @param pChannel the channel to send the noteoff.
 * @param pPitch the note to turn off.
 */
void noteOff(byte pChannel, byte pPitch){
  pChannel += noteOFF-1;
  midiSend(pChannel, pPitch, 0);
}

/**
 * Method to send the midi message.
 * @param statusByte the status midi command.
 * @param dataByte1 the first data byte.
 * @param dataByte2 the second data byte.
 * @return void
 */
void midiSend(byte statusByte, byte dataByte1, byte dataByte2) {
#ifdef debug
  Serial.println("Begin Message.");
  Serial.println(statusByte, BIN);
  Serial.println(dataByte1, BIN);
  Serial.println(dataByte2, BIN);
  Serial.println("End Message.");
#else
  Serial.write(statusByte);
  Serial.write(dataByte1);
  Serial.write(dataByte2);
#endif
}

/**
 * Main loop method
 * @param none
 * @return void
 */
void loop() {
#ifdef debug
  Serial.println("Loop start");
#endif
  readAnalog();
  readKeys();
  readButtons();
#ifdef debug
  Serial.println("Loop pass");
#endif
}










