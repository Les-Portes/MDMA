// Kontroller by Karg (Timm Schlegelmilch)
// Control your Kaossilator Pro (+) with standard midi notes
//
// Detailed instructions are published on:
// http://karg-music.blogspot.de
//
// Licenced under Creative Commons Attribution-ShareAlike 4.0
// http://creativecommons.org/licenses/by-sa/4.0/

//
//  Note that we are translating notes into coordinates on the touch pad's XY-axis of your Kaossilator Pro (+)
//  This means: - the scale set on the Kaossilator will be respected
//              - the octave range set on the Kaossilator will will modulate the notes played
//              - for a one-to-one musical mapping between key pressed and note played, set the octave range to full on your Kaossilator
//
// ------------------------------------------------------------
// les-portes update - 2016/09/18
// - exact note mapping for 4 OCT / Chromatic scale
// - add portamento
// - add tracking for two simultaneous keys
// Known bugs & limitations
//  - only 4 oct mode supported for now
//  - limited to two notes memory
//  - no portamento interruption if key is released, which can induce a lag in some cases
//  - pitch bend is not truly accurate
//  - not tested on USB, I only use DIN
// ------------------------------------------------------------

/* ---( Config Start - change to your likings )-------------------------------------------------------------------------------- */

// un-/comment to enable/disable MIDI DINs and USB
#define DIN
//#define USB

#define INPUT_MIDI_CHANNEL 1            // MIDI channel of the input device (keyboard etc.) sending notes

// change the numbers according to the settings in your Kaoss device
#define KAOSS_MIDI_CHANNEL 2            // MIDI channel for the Kaoss device
#define KAOSS_CC_PAD       92           // pad on/off control change # (check the manual for more information)
#define KAOSS_CC_X         12           // pad on/off control change # (check the manual for more information)
#define KAOSS_CC_Y         13           // pad on/off control change # (check the manual for more information)


#define PITCH_BEND_RANGE 2 //in number of notes, 12 = 1 octave
#define LOWEST_NOTE 36
#define HIGHEST_NOTE 85

#define PORTAMENTO
#define GLIDE_TIME 1000000

// uncomment to enable, comment to disable
#define X_PITCH_CHANGE                  // pitch bend wheel modulates the note
//#define Y_VELOCITY                    // touch pad Y-axis: note velocity is translated into Y axis value
#define Y_CONTROL_CHANGE   1            // touch pad Y-axis: chontrol change # that is translated into Y axes value (e.g. 1 for Modulation Wheel)
#define Y_AFTERTOUCH                    // touch pad Y-axis: aftertouch modulates Y axis value

#define NOTE_INDICATION                 // lights up the LED while a note is played


/* ---( Config End - from here on it is better if you know what you are doing )------------------------------------------------ */
#ifdef DIN
#include <MIDI.h>
//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

byte  xPad  = 0;
byte  xPadPrevious  = 0;
byte  noteMemory[2][3] = {{0, 0, 0}, {0, 0, 0}};
byte  yPad  = 0;
int   pitchChange = 0;
int   afterTouch  = 0;

const int TAB_KAOSS_MAPPING_1OCT[] = { 1, 10, 19, 27, 36, 44, 53, 61, 70, 78, 87, 95, 104, 112};
const int TAB_KAOSS_MAPPING_4OCT[] = { 1, 4, 7, 9, 12, 15, 17, 21, 23 , 26, 29, 31,
                                       34, 36, 38, 41, 43, 46, 49, 52, 55, 57, 60, 62,
                                       65, 67, 70, 72, 75, 77, 80, 82, 85, 88, 90, 93,
                                       96, 99, 102, 105, 107, 110, 112, 115, 117, 120, 123, 125,
                                       127
                                     };

// will convert from a MIDI note to the correct CC for Kaoss
byte toScale(byte note) {
  int centeredNote = note - LOWEST_NOTE;
  return TAB_KAOSS_MAPPING_4OCT[centeredNote];
}

//add note to memory
void memorize(byte note, byte velocity) {
  if(noteMemory[0][0] == note)
    return;  
  noteMemory[1][0] = noteMemory[0][0];
  noteMemory[1][1] = noteMemory[0][1];
  noteMemory[0][0] = note;
  noteMemory[0][1] = velocity;
}

//remove latest note from memory
void forgetLatest() {
  noteMemory[0][0] = noteMemory[1][0];
  noteMemory[0][1] = noteMemory[1][1];
  noteMemory[1][0] = 0;
  noteMemory[1][1] = 0;
}

//remove oldest note from memory
void forgetOldest() {
  noteMemory[1][0] = 0;
  noteMemory[1][1] = 0;
}

void sendNote(byte x, byte y) {
#ifdef NOTE_INDICATION
        digitalWrite(13, HIGH);
#endif

#ifdef DIN
  MIDI.sendControlChange(KAOSS_CC_X, constrain(x, 0, 127), KAOSS_MIDI_CHANNEL);
  MIDI.sendControlChange(KAOSS_CC_Y, constrain(y, 0, 127), KAOSS_MIDI_CHANNEL);
  MIDI.sendControlChange(KAOSS_CC_PAD, 127, KAOSS_MIDI_CHANNEL);
#endif
#ifdef USB
  usbMIDI.sendControlChange(KAOSS_CC_X, constrain(x, 0, 127), KAOSS_MIDI_CHANNEL);
  usbMIDI.sendControlChange(KAOSS_CC_Y, constrain(y, 0, 127), KAOSS_MIDI_CHANNEL);
  usbMIDI.sendControlChange(KAOSS_CC_PAD, 127, KAOSS_MIDI_CHANNEL);
#endif
}

void clearNote() {
#ifdef NOTE_INDICATION
  digitalWrite(13, LOW);
#endif
#ifdef DIN
  MIDI.sendControlChange(KAOSS_CC_PAD, 0, KAOSS_MIDI_CHANNEL);
#endif
#ifdef USB
  usbMIDI.sendControlChange(KAOSS_CC_PAD, 0, KAOSS_MIDI_CHANNEL);
#endif
}

void OnNoteOn(byte channel, byte note, byte velocity) {
  int i;
  if (channel == INPUT_MIDI_CHANNEL) {
    memorize(note, velocity); //start by memorizing it : if it disappears, we have to stop the process
    xPad = toScale(note);
#ifdef Y_VELOCITY
    yPad = velocity;
#endif
    if (note >= LOWEST_NOTE && note < HIGHEST_NOTE && velocity) {
#ifdef PORTAMENTO
        if (xPadPrevious == 0 || xPadPrevious == xPad)
          sendNote(xPad + pitchChange, yPad + afterTouch);
        else if ( xPad > xPadPrevious)
          for (i = 0; i <= xPad - xPadPrevious && noteMemory[0][0] == note ; i++) {
            sendNote(xPadPrevious + i + pitchChange, yPad + afterTouch);
            delayMicroseconds(GLIDE_TIME);
          }
        else
          for (i = 0; i <= xPadPrevious - xPad && noteMemory[0][0] == note ; i++) {
            sendNote(xPadPrevious -  i + pitchChange, yPad + afterTouch);
            delayMicroseconds(GLIDE_TIME);
          }
        xPadPrevious = xPad;
#endif
#ifndef PORTAMENTO
        sendNote(xPad + pitchChange, yPad + afterTouch);
#endif        
      } else {
        //out of scope notes act as MIDI cut all
        clearNote();
      }
  }
}


void OnNoteOff(byte channel, byte note, byte velocity) {
  if (channel == INPUT_MIDI_CHANNEL) {
    if (note == noteMemory[0][0]) { //current note released
      forgetLatest();
      if (noteMemory[0][0] == 0)
        clearNote(); //no note left in memory, go silent
      else 
        OnNoteOn(channel, noteMemory[0][0], noteMemory[0][1]);
    }
    else if (note == noteMemory[1][0]) //stored note released
      forgetOldest();
  }
}


void OnPitchChange(byte channel, int pitch) {
  double pitchRatio = 0;
  if (channel == INPUT_MIDI_CHANNEL) {
#ifdef X_PITCH_CHANGE
#ifdef DIN
  pitchRatio = pitch / 8192.0;
  pitchChange = round(pitchRatio * toScale(PITCH_BEND_RANGE + LOWEST_NOTE));
  MIDI.sendControlChange(KAOSS_CC_X, constrain(xPad + pitchChange, 0, 127), KAOSS_MIDI_CHANNEL);
#endif
#ifdef USB
    pitchRatio = pitch / 8192.0 -1.0;
    pitchChange = round(pitchRatio * toScale(PITCH_BEND_RANGE + LOWEST_NOTE));
    usbMIDI.sendControlChange(KAOSS_CC_X, constrain(xPad + pitchChange, 0, 127), KAOSS_MIDI_CHANNEL);
#endif
#endif
  }
}


void OnControlChange(byte channel, byte control, byte value) {
  if (channel == INPUT_MIDI_CHANNEL) {
#ifdef Y_CONTROL_CHANGE
    if (control == Y_CONTROL_CHANGE) {
      yPad = value;
#ifdef DIN
      MIDI.sendControlChange(KAOSS_CC_Y, yPad, KAOSS_MIDI_CHANNEL);
#endif
#ifdef USB
      usbMIDI.sendControlChange(KAOSS_CC_Y, yPad, KAOSS_MIDI_CHANNEL);
#endif
    }
#endif
  }
}


void OnAfterTouch(byte channel, byte value) {
  if (channel == INPUT_MIDI_CHANNEL) {
#ifdef Y_AFTERTOUCH
    afterTouch = value;
#ifdef DIN
    MIDI.sendControlChange(KAOSS_CC_Y, constrain(yPad + afterTouch, 0, 127), KAOSS_MIDI_CHANNEL);
#endif
#ifdef USB
    usbMIDI.sendControlChange(KAOSS_CC_Y, constrain(yPad + afterTouch, 0, 127), KAOSS_MIDI_CHANNEL);
#endif
#endif
  }
}



void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);

#ifdef DIN
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(OnNoteOn);
  MIDI.setHandleNoteOff(OnNoteOff);
#ifdef X_PITCH_CHANGE
  MIDI.setHandlePitchBend(OnPitchChange);
#endif
#ifdef Y_CONTROL_CHANGE
  MIDI.setHandleControlChange(OnControlChange);
#endif
#ifdef Y_AFTERTOUCH
  MIDI.setHandleAfterTouchChannel(OnAfterTouch);
#endif
#endif

#ifdef USB
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
#ifdef X_PITCH_CHANGE
  usbMIDI.setHandlePitchChange(OnPitchChange);
#endif
#ifdef Y_CONTROL_CHANGE
  usbMIDI.setHandleControlChange(OnControlChange);
#endif
#ifdef Y_AFTERTOUCH
  usbMIDI.setHandleAfterTouch(OnAfterTouch);
#endif
#endif
}

void loop() {
#ifdef DIN
  MIDI.read();
#endif
#ifdef USB
  usbMIDI.read();
#endif
}
