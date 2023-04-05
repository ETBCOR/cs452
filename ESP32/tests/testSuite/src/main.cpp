#include <main.h>
#include <Arduino.h>

void setup() {
  //button_test();
  //sev_seg_test();
  dip_test();
}

void loop() {}

void button_test() {
  pinMode(BUTTONS1, INPUT);
  pinMode(BUTTONS2, INPUT);
  pinMode(BUTTONS3, INPUT);

  Serial.begin(115200);
  Serial.println("\nV3-DIP8Test");

  while (1) {
    Serial.print(digitalRead(BUTTONS1));
    Serial.print(digitalRead(BUTTONS2));
    Serial.println(digitalRead(BUTTONS3));
    delay(500);
  }
}

void sev_seg_test() {
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

  while (true) {
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
}

void dip_test() {
  pinMode(DIP0, INPUT);
  pinMode(DIP1, INPUT);
  pinMode(DIP2, INPUT);
  pinMode(DIP3, INPUT);
  pinMode(DIP4, INPUT);
  pinMode(DIP5, INPUT);
  pinMode(DIP6, INPUT);
  pinMode(DIP7, INPUT);

  Serial.begin(115200);
  Serial.println("\nV2-DIP8Test");

  while (true) {
    Serial.print(digitalRead(DIP0));
    Serial.print(digitalRead(DIP1));
    Serial.print(digitalRead(DIP2));
    Serial.print(digitalRead(DIP3));
    Serial.print(digitalRead(DIP4));
    Serial.print(digitalRead(DIP5));
    Serial.print(digitalRead(DIP6));
    Serial.println(digitalRead(DIP7));
    delay(500);
  }
}
