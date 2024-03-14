/*
   This ESP32 code is created by esp32io.com

   This ESP32 code is released in the public domain

   For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-led-blink
*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin GPIO18 as an output.
  pinMode(23, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(32, OUTPUT);

}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(23, HIGH); // turn the LED on
  digitalWrite(22, HIGH); // turn the LED on
  digitalWrite(19, HIGH); // turn the LED on
  digitalWrite(18, HIGH); // turn the LED on
  digitalWrite(33, HIGH); // turn the LED on
  digitalWrite(25, HIGH); // turn the LED on
  digitalWrite(26, HIGH); // turn the LED on
  digitalWrite(32, HIGH); // turn the LED on
  delay(5000);             // wait for 500 milliseconds
  digitalWrite(23, LOW);  // turn the LED off
  digitalWrite(22, LOW);  // turn the LED off
  digitalWrite(19, LOW);  // turn the LED off
  digitalWrite(18, LOW);  // turn the LED off
  digitalWrite(33, LOW);  // turn the LED off
  digitalWrite(25, LOW);  // turn the LED off
  digitalWrite(26, LOW);  // turn the LED off
  digitalWrite(32, LOW);  // turn the LED off
  delay(5000);             // wait for 500 milliseconds
}
