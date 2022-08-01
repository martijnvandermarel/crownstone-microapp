//
// An example to show a few functions being implemented.
//

#include <Arduino.h>

// Show how a counter is incremented
static int counter = 100;

microapp_service_data_t serviceData;

volatile byte state = LOW;
volatile byte state2 = LOW;

//
// The blink function will be called on interrupt CHANGE. This means when you press and when you release a button.
// Therefore two state variables are used to only toggle the state every two interrupts.
void blink() {
	Serial.println("Toggle");
	state2 = !state2;
	if (state2 == LOW) {
		state = !state;
		digitalWrite(LED2_PIN, state);
	}
}

//
// An example of a setup function.
//
void setup() {

	// We can "start" serial.
	Serial.begin();

	// We can use if(Serial), although this will always return true now (might be different in release mode).
	if (!Serial) return;

	// We can also write integers.
	Serial.println(counter);

	// Set the UUID of this microapp.
	serviceData.appUuid = 0x1234;
	serviceData.dlen = 1;

	// Set LED pin to OUTPUT, so we can write.
	pinMode(LED2_PIN, OUTPUT);

	// Join the i2c bus
	Wire.begin();

	// Set interrupt handler
	pinMode(BUTTON2_PIN, INPUT_PULLUP);
	if (!attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), blink, CHANGE)) {
		Serial.println("Setting button interrupt failed");
	}

}

void i2c() {
	// We can use local variables, also before and after delay() calls.
	// int test = 1;
	const byte address = 0x18;

	// Start transmission over i2c bus
	Wire.beginTransmission(address);
	Wire.write(5);
	Wire.endTransmission();

	Serial.print("I2C read request: ");

	// Request a few bytes from device at given address
	Wire.requestFrom(address, 2);

	while (Wire.available()) {
		char c = Wire.read();
		Serial.write(c);
		Serial.write("  ");
	}
	Serial.println(". ");
}

//
// A dummy loop function.
//
void loop() {
	// We are able to use static variables.
	counter++;

	if (counter % 5 == 0) {
#ifdef CHECK_TWI
		i2c();
#endif
		if (state == LOW) {
			Serial.println("State down");
		} else {
			Serial.println("State up");
		}

	}

	if (counter % 10 == 0) {
		Serial.println("Delay");
		// This is in ms
		delay(10000);

	}
	// Show counter.
	Serial.println(counter);

	// Let's advertise the counter value in the service data.
	serviceData.data[0] = counter;
	SerialServiceData.write(&serviceData);
}
