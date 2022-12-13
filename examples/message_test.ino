#include <Arduino.h>
#include <Message.h>


/**
 * Test for the Message class
 */

void setup() {
	Serial.println("Message test");

	if (!Message.begin()) {
		Serial.println("Message begin failed");
	}
}

void loop() {
	char msg[12] = "hello world";
	Message.write(msg, sizeof(msg));
	delay(1000);
}
