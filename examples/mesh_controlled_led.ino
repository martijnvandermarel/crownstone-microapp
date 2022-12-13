#include <Arduino.h>
#include <Mesh.h>

// This app allows controlling the blinking frequency of its LED
// It is meant to showcase listening to incoming mesh messages

// lets microapp differentiate between different kinds of microapp mesh messages
const uint8_t MICROAPP_MESH_LED_CONTROL_OPCODE = 0x44;
// period for which LED should blink (100ms units). Can be set through mesh messages
uint8_t blinkPeriod = 10; // = 10 * 100ms = 1s
boolean ledState = LOW;

void onMeshMessage(MeshMsg msg) {
	if (msg.dataPtr[0] != MICROAPP_MESH_LED_CONTROL_OPCODE) {
		return;
	}
	blinkPeriod = msg.dataPtr[1];
}

// In setup, configure button and LED GPIO pins and register callback
void setup() {
	Serial.begin();
	Serial.println("Mesh-controlled LED");

	// Configure LED pin as output
	pinMode(LED2_PIN, OUTPUT);

	// Set interrupt handler
	if (!Mesh.listen()) {
		Serial.println("Mesh.listen() failed");
	}
	Mesh.setIncomingMeshMsgHandler(onMeshMessage);

}

void loop() {
	ledState = !ledState;
	digitalWrite(LED2_PIN, ledState);
	delay(100 * blinkPeriod);
}
