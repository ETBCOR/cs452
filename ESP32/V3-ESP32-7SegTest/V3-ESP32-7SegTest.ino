/*
 7 Segment display only partially works on ESP32
*/
#define LED_BUILTIN 13

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

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(SevenSegA, OUTPUT);
  pinMode(SevenSegB, OUTPUT);
  pinMode(SevenSegC, OUTPUT);
  pinMode(SevenSegD, OUTPUT);
  pinMode(SevenSegE, OUTPUT);
  pinMode(SevenSegF, OUTPUT);
  pinMode(SevenSegG, OUTPUT);
  pinMode(SevenSegDP, OUTPUT);

  pinMode(SevenSegCC1, OUTPUT);
  pinMode(SevenSegCC2, OUTPUT);



}

// the loop function runs over and over again forever
void loop() {
  // CC1 Zero
  // CC2 Zero
  digitalWrite(SevenSegCC1, LOW);
  digitalWrite(SevenSegCC2, LOW);

  delay(500);                       // wait for a second
  digitalWrite(SevenSegA, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegA, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegB, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegB, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegC, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegC, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegD, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegD, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegE, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegE, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegF, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegF, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegG, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegG, LOW);    // turn the LED On by making the voltage LOW

  delay(500);                       // wait for a second
  digitalWrite(SevenSegDP, HIGH);    // turn the LED On by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(SevenSegDP, LOW);    // turn the LED On by making the voltage LOW
}
