#include <Arduino.h>
#include <Message.h>


/**
 * Test for the Message class
 */

uint8_t buf[MICROAPP_SDK_MESSAGE_SEND_MSG_MAX_SIZE];
const char* dots = "...";

void flashLED() {
	digitalWrite(LED2_PIN, HIGH);
	delay(100);
	digitalWrite(LED2_PIN, LOW);
}

void setup() {
	Serial.println("Message test");

	if (!Message.begin()) {
		Serial.println("Message begin failed");
	}

	pinMode(LED2_PIN, OUTPUT);
}

void loop() {
	uint8_t bytesAvailable = Message.available();
	uint8_t bytesToSend;
	if (bytesAvailable > 0) {
		Message.readBytes(buf, bytesAvailable);
		flashLED();
		Serial.println(buf, bytesAvailable);
		// repeat message since it's an echo...if there is space
		if (bytesAvailable * 2 + strlen(dots) < MICROAPP_SDK_MESSAGE_SEND_MSG_MAX_SIZE) {
			memcpy(buf + bytesAvailable, dots, strlen(dots));
			memcpy(buf + bytesAvailable + strlen(dots), buf, bytesAvailable);
			bytesToSend = bytesAvailable * 2 + strlen(dots);
		}
		else {
			bytesToSend = bytesAvailable;
		}
		Message.write(buf, bytesToSend);
	}
}
