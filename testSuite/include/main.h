#include <Arduino.h>


// ---- BUTTON TEST ---- //
void button_test();

// GP19
#define BUTTONS1 19
// GP09
#define BUTTONS2 15
// GP08
#define BUTTONS3 32

// ---- SEVEN SEGMENT TEST ---- //
void sev_seg_test();

// GP11
#define SevenSegCC1 27
// GP10
#define SevenSegCC2 33
// GP26
#define SevenSegA 26
// GP27
#define SevenSegB 25
// GP29 - INPUT ONLY - Doesn't drive the Segment
#define SevenSegC 39
// GP18
#define SevenSegD 5
// GP25
#define SevenSegE 4
// GP07
#define SevenSegF 14
// GP28 - INPUT ONLY - Doesn't drive the Segment
#define SevenSegG 34
// GP24 - INPUT ONLY - Doesn't drive the Segment
#define SevenSegDP 36  


// ---- DIPSWITCH TEST ---- //
void dip_test();

// GP06
#define DIP0 37
// GP07
#define DIP1 14
// SCL
#define DIP2 20
// SDA
#define DIP3 22
// GP13
#define DIP4 13
// GP26
#define DIP5 26
// GP00
#define DIP6 8
// GP01
#define DIP7 7
