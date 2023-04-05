// GP06
#define DIP0 21
// GP07
#define DIP1 14
// SCL
#define DIP2 22
// SDA
#define DIP3 23
// GP13
#define DIP4 13
// GP26
#define DIP5 26
// GP00
#define DIP6 17
// GP01
#define DIP7 16





void setup() {
  // put your setup code here, to run once:
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
}

void loop() {
  // put your main code here, to run repeatedly:

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
