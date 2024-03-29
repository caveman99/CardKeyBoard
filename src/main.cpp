#include <RingBuf.h>
RingBuf<unsigned char, 32> txBuf;

#include <EEPROM.h>

byte brightness = 40, Bus1, Bus2;

#ifndef AVR_LEONARDO
#include <Adafruit_NeoPixel.h>
// TODO: Does this need to be changed for ATMega8?
#define PIN 13
#define NUMPIXELS 1

Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#else
class Pixels {
public:
  void begin() {}
  int Color(int, int, int) { return 0; }
  void setPixelColor(int, int) {}
  void setBrightness(int) {}
  void show() {}
  void clear() {}
};
Pixels pixels;
#endif

#include <Wire.h>

#define SLAVE_I2C_ADDRESS_DEFAULT 0x5F
#define KEYPAD_VERSION 0x12

#define Set_Bit(val, bitn) (val |= (1 << (bitn)))
#define Clr_Bit(val, bitn) (val &= ~(1 << (bitn)))
#define Get_Bit(val, bitn) (val & (1 << (bitn)))

#ifndef AVR_LEONARDO

//       d0    d1 d2 d3 d4 d5 d6 d7 d8 d9 d10 d11
// A3： esc     1  2  3  4  5  6  7  8  9  0  del
// A2:  tab     q  w  e  r  t  y  u  i  o  p
// A1: left    up  a  s  d  f  g  h  j  k  l  enter
// A0: down right  z  x  c  v  b  n  m  ,  .  space
// sym: d15
// shift: d12
// fn: d14

unsigned char KeyMap[48][7] = {
    // nor, shift,long_shift, sym,long_sym,fn,long_fn,
    {27, 27, 27, 27, 27, 128, 128},        // esc
    {'1', '1', '1', '!', '!', 129, 129},   // 1
    {'2', '2', '2', '@', '@', 130, 130},   // 2
    {'3', '3', '3', '#', '#', 131, 131},   // 3
    {'4', '4', '4', '$', '$', 132, 132},   // 4
    {'5', '5', '5', '%', '%', 133, 133},   // 5
    {'6', '6', '6', '^', '^', 134, 134},   // 6
    {'7', '7', '7', '&', '&', 135, 135},   // 7
    {'8', '8', '8', '*', '*', 136, 136},   // 8
    {'9', '9', '9', '(', '(', 137, 137},   // 9
    {'0', '0', '0', ')', ')', 138, 138},   // 0
    {8, 127, 127, 8, 8, 139, 139},         // del
    {9, 9, 9, 9, 9, 140, 140},             // tab
    {'q', 'Q', 'Q', '{', '{', 141, 141},   // q
    {'w', 'W', 'W', '}', '}', 142, 142},   // w
    {'e', 'E', 'E', '[', '[', 143, 143},   // e
    {'r', 'R', 'R', ']', ']', 144, 144},   // r
    {'t', 'T', 'T', '/', '/', 145, 145},   // t
    {'y', 'Y', 'Y', '\\', '\\', 146, 146}, // y
    {'u', 'U', 'U', '|', '|', 147, 147},   // u
    {'i', 'I', 'I', '~', '~', 148, 148},   // i
    {'o', 'O', 'O', '\'', '\'', 149, 149}, // o
    {'p', 'P', 'P', '"', '"', 150, 150},   // p
    {0, 0, 0, 0, 0, 0, 0},                 //  no key
    {180, 180, 180, 180, 180, 152, 152},   // LEFT
    {181, 181, 181, 181, 181, 153, 153},   // UP
    {'a', 'A', 'A', ';', ';', 154, 154},   // a
    {'s', 'S', 'S', ':', ':', 155, 155},   // s
    {'d', 'D', 'D', '`', '`', 156, 156},   // d
    {'f', 'F', 'F', '+', '+', 157, 157},   // f
    {'g', 'G', 'G', '-', '-', 158, 158},   // g
    {'h', 'H', 'H', '_', '_', 159, 159},   // h
    {'j', 'J', 'J', '=', '=', 160, 160},   // j
    {'k', 'K', 'K', '?', '?', 161, 161},   // k
    {'l', 'L', 'L', 0, 0, 162, 162},       // l
    {13, 13, 13, 13, 13, 163, 163},        // enter
    {182, 182, 182, 182, 182, 164, 164},   // DOWN
    {183, 183, 183, 183, 183, 165, 165},   // RIGHT
    {'z', 'Z', 'Z', 0, 0, 166, 166},       // z
    {'x', 'X', 'X', 0, 0, 167, 167},       // x
    {'c', 'C', 'C', 0, 0, 168, 168},       // c
    {'v', 'V', 'V', 0, 0, 169, 169},       // v
    {'b', 'B', 'B', 0, 0, 170, 170},       // b
    {'n', 'N', 'N', 0, 0, 171, 171},       // n
    {'m', 'M', 'M', 0, 0, 172, 172},       // m
    {',', ',', ',', '<', '<', 173, 173},   //,
    {'.', '.', '.', '>', '>', 174, 174},   //.
    {' ', ' ', ' ', ' ', ' ', 175, 175}    // space
};

#else

// Mapping for Leonardo/Beetle with a 3x3 layout

//       d9   d10   d11
// A2:  esc   tab   del
// A1: left    up enter
// A0: down right space

unsigned char KeyMap[9][1] = {
    {27},  // esc
    {9},   // tab
    {8},   // del
    {180}, // LEFT
    {181}, // UP
    {13},  // enter
    {182}, // DOWN
    {183}, // RIGHT
    {' '}  // space
};

#endif

#define shiftPressed (PINB & 0x10) != 0x10
#define symPressed (PINB & 0x80) != 0x80
#define fnPressed (PINB & 0x40) != 0x40
int _shift = 0, _fn = 0, _sym = 0, idle = 0;
unsigned char KEY = 0, OUT = 0;
bool hadPressed = false;
int Mode = 0; // 0->normal, 1->shift, 2->long_shift, 3->sym, 4->long_shift,
              // 5->fn, 6->long_fn
void flashOn() {
  pixels.setPixelColor(0, pixels.Color(3, 3, 3));
  pixels.setBrightness(brightness);
  pixels.show();
}

void flashOff() {
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.setBrightness(brightness);
  pixels.show();
}

void requestEvent() {
  while (txBuf.pop(OUT))
    Wire.write(OUT);
}

void receiveEvent(int numBytes) {
  int command = Wire.read();
  switch (command) {
  case 0x02:
    Wire.write(Bus1);
    break;
  case 0x03:
    if (numBytes == 1) {
      Bus1 = Wire.read();
      EEPROM.update(0, Bus1);
    }
    break;
  case 0x04:
    Wire.write(KEYPAD_VERSION);
    break;
  case 0x11:
    pixels.clear();
    break;

  case 0x12:
    brightness = Wire.read();
    break;

  case 0x13:
    if (numBytes == 4) {
      int r = Wire.read();
      int g = Wire.read();
      int b = Wire.read();
      pixels.setPixelColor(0, pixels.Color(r, g, b));
      pixels.setBrightness(brightness);
      pixels.show();
    }
    break;

  default: // 0x01, -1 or any other command sent
    while (txBuf.pop(OUT))
      Wire.write(OUT);
    break;
  }
}

void setup() {
#ifndef AVR_LEONARDO
  pinMode(A3, OUTPUT);
#endif
  pinMode(A2, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
#ifndef AVR_LEONARDO
  digitalWrite(A3, LOW);
  DDRB = 0x00;
  PORTB = 0xff;
  DDRD = 0x00;
  PORTD = 0xff;
#else
  DDRB &= 31;
  PORTB |= 224;
#endif

  EEPROM.get(1, Bus1);
  EEPROM.get(1, Bus2);
  if (Bus2 != SLAVE_I2C_ADDRESS_DEFAULT) {
    Bus1 = SLAVE_I2C_ADDRESS_DEFAULT;
    Bus2 = SLAVE_I2C_ADDRESS_DEFAULT;
    EEPROM.update(0, Bus1);
    EEPROM.update(1, Bus2);
  }

  pixels.begin();
  for (int j = 0; j < 3; j++) {
    for (int i = 0; i < 30; i++) {
      pixels.setPixelColor(0, pixels.Color(i, i, i));
      pixels.setBrightness(brightness);
      pixels.show();
      delay(6);
    }
    for (int i = 30; i > 0; i--) {
      pixels.setPixelColor(0, pixels.Color(i, i, i));
      pixels.setBrightness(brightness);
      pixels.show();
      delay(6);
    }
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.setBrightness(brightness);
  pixels.show();
  Wire.begin(Bus1);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

#ifndef AVR_LEONARDO
unsigned char GetInput() {
  digitalWrite(A3, LOW);
  digitalWrite(A2, HIGH);
  digitalWrite(A1, HIGH);
  digitalWrite(A0, HIGH);
  delay(2);
  switch (PIND) {
  case 254:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 1;
    break;
  case 253:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 2;
    break;
  case 251:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 3;
    break;
  case 247:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 4;
    break;
  case 239:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 5;
    break;
  case 223:
    while (PIND != 0xff) {
      flashOn();
      //   delay(1);
    }
    flashOff();
    hadPressed = true;
    return 6;
    break;
  case 191:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 7;
    break;
  case 127:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 8;
    break;
  }
  switch (PINB) {
  case 222:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 9;
    break;
  case 221:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 10;
    break;
  case 219:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 11;
    break;
  case 215:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 12;
    break;
  }

  digitalWrite(A3, HIGH);
  digitalWrite(A2, LOW);
  digitalWrite(A1, HIGH);
  digitalWrite(A0, HIGH);
  delay(2);
  switch (PIND) {
  case 254:
    while (PIND != 0xff) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 13;
    break;
  case 253:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 14;
    break;
  case 251:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 15;
    break;
  case 247:
    while (PIND != 0xff) {
      flashOn();
      delay(1);
    }
    flashOff();
    hadPressed = true;
    return 16;
    break;
  case 239:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 17;
    break;
  case 223:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 18;
    break;
  case 191:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 19;
    break;
  case 127:
    while (PIND != 0xff) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 20;
    break;
  }
  switch (PINB) {
  case 222:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 21;
    break;
  case 221:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 22;
    break;
  case 219:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 23;
    break;
  case 215:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 24;
    break;
  }

  digitalWrite(A3, HIGH);
  digitalWrite(A2, HIGH);
  digitalWrite(A1, LOW);
  digitalWrite(A0, HIGH);
  delay(2);
  switch (PIND) {
  case 254:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 25;
    break;
  case 253:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 26;
    break;
  case 251:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 27;
    break;
  case 247:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 28;
    break;
  case 239:
    while (PIND != 0xff) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 29;
    break;
  case 223:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 30;
    break;
  case 191:
    while (PIND != 0xff) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 31;
    break;
  case 127:
    while (PIND != 0xff) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 32;
    break;
  }
  switch (PINB) {
  case 222:
    while (PINB != 223) {
      flashOn();
      //  delay(1);
    }
    flashOff();
    hadPressed = true;
    return 33;
    break;
  case 221:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 34;
    break;
  case 219:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 35;
    break;
  case 215:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 36;
    break;
  }

  digitalWrite(A3, HIGH);
  digitalWrite(A2, HIGH);
  digitalWrite(A1, HIGH);
  digitalWrite(A0, LOW);
  delay(2);
  switch (PIND) {
  case 254:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 37;
    break;
  case 253:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 38;
    break;
  case 251:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 39;
    break;
  case 247:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 40;
    break;
  case 239:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 41;
    break;
  case 223:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 42;
    break;
  case 191:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 43;
    break;
  case 127:
    while (PIND != 0xff) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 44;
    break;
  }
  switch (PINB) {
  case 222:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 45;
    break;
  case 221:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 46;
    break;
  case 219:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 47;
    break;
  case 215:
    while (PINB != 223) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 48;
    break;
  }
  hadPressed = false;
  return 255;
}
#else
unsigned char GetInput() {
  digitalWrite(A2, LOW);
  digitalWrite(A1, HIGH);
  digitalWrite(A0, HIGH);
  delay(2);
  uint8_t latch = PINB & 224;
  switch (latch) {
  case 192:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 1;
    break;
  case 160:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 2;
    break;
  case 96:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 3;
    break;
  }
  digitalWrite(A2, HIGH);
  digitalWrite(A1, LOW);
  digitalWrite(A0, HIGH);
  delay(2);
  latch = PINB & 224;
  switch (latch) {
  case 192:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 4;
    break;
  case 160:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 5;
    break;
  case 96:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 6;
    break;
  }
  digitalWrite(A2, HIGH);
  digitalWrite(A1, HIGH);
  digitalWrite(A0, LOW);
  delay(2);
  latch = PINB & 224;
  switch (latch) {
  case 192:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 7;
    break;
  case 160:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 8;
    break;
  case 96:
    while (latch != 224) {
      flashOn();
    }
    flashOff();
    hadPressed = true;
    return 9;
    break;
  }
  hadPressed = false;
  return 255;
}
#endif

void loop() {
  if (shiftPressed) {
    _sym = 0;
    _fn = 0;
    idle = 0;
    while (shiftPressed)
      delay(1);
    if (_shift == 0) {
      delay(200);
      if (shiftPressed) {
        while (shiftPressed)
          delay(1);
        _shift = 2;
        Mode = 2;
      } else {
        _shift = 1;
        Mode = 1;
      }
    } else {
      delay(200);
      if (shiftPressed) {
        while (shiftPressed)
          delay(1);
        if (_shift == 2) {
          Mode = 0;
          _shift = 0;
        } else {
          Mode = 2;
          _shift = 2;
        }
      } else {
        Mode = 0;
        _shift = 0;
      }
    }
  }

  if (symPressed) {
    _shift = 0;
    _fn = 0;
    idle = 0;
    while (symPressed)
      delay(1);
    if (_sym == 0) {
      delay(200);
      if (symPressed) {
        while (symPressed)
          delay(1);
        _sym = 2;
        Mode = 4;
      } else {
        _sym = 1;
        Mode = 3;
      }
    } else {
      delay(200);
      if (symPressed) {
        while (symPressed)
          delay(1);
        if (_sym == 2) {
          Mode = 0;
          _sym = 0;
        } else {
          Mode = 4;
          _sym = 2;
        }
      } else {
        Mode = 0;
        _sym = 0;
      }
    }
  }

  if (fnPressed) {
    _sym = 0;
    _shift = 0;
    idle = 0;
    while (fnPressed)
      delay(1);
    if (_fn == 0) {
      delay(200);
      if (fnPressed) {
        while (fnPressed)
          delay(1);
        _fn = 2;
        Mode = 6;
      } else {
        _fn = 1;
        Mode = 5;
      }
    } else {
      delay(200);
      if (fnPressed) {
        while (fnPressed)
          delay(1);
        if (_fn == 2) {
          Mode = 0;
          _fn = 0;
        } else {
          Mode = 6;
          _fn = 2;
        }
      } else {
        Mode = 0;
        _fn = 0;
      }
    }
  }

  switch (Mode) {
  case 0: // normal
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    break;
  case 1: // shift
    if ((idle / 6) % 2 == 1)
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    else
      pixels.setPixelColor(0, pixels.Color(5, 0, 0));
    break;
  case 2: // long_shift
    pixels.setPixelColor(0, pixels.Color(5, 0, 0));
    break;
  case 3: // sym
    if ((idle / 6) % 2 == 1)
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    else
      pixels.setPixelColor(0, pixels.Color(0, 5, 0));
    break;
  case 4: // long_sym
    pixels.setPixelColor(0, pixels.Color(0, 5, 0));
    break;
  case 5: // fn
    if ((idle / 6) % 2 == 1)
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    else
      pixels.setPixelColor(0, pixels.Color(0, 0, 5));
    break;
  case 6: // long_fn
    pixels.setPixelColor(0, pixels.Color(0, 0, 5));
    break;
  }

  pixels.setBrightness(brightness);
  pixels.show(); // This sends the updated pixel color to the hardware.
  if (!hadPressed) {
    // feed the ringbuffer instead of the single key
    KEY = GetInput();
    if (hadPressed) {
      txBuf.push(KeyMap[KEY - 1][Mode]);
      if ((Mode == 1) || (Mode == 3) || (Mode == 5)) {
        Mode = 0;
        _shift = 0;
        _sym = 0;
        _fn = 0;
      }
      hadPressed = false;
    }
  }
  idle++;
  delay(10);
}
