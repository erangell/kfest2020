/*
  Serial Event example
 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and
 clears it.
Adapted from 9 May 2011 by Tom Igoe
 http://www.arduino.cc/en/Tutorial/SerialEvent
 */

void setup() {
  //onboard LED will signal transmission of lyric events to Replica 1
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

  // initialize serial for Apple //c+
  Serial.begin(19200);
  //Serial.println("Hello Apple //c+");
  
  //Initialize Serial1 for MIDI
  Serial1.begin(31250);  
  //Serial.println("Opened MIDI OUT");  
}

void loop() {
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */

void serialEvent() {
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      Serial1.print(inChar); // send MIDI out
    } // while
}

