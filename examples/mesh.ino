#include <Mesh.h>
#include <Serial.h>

/**
 * A microapp example for transmitting and receiving mesh messages
 *
 * Two roles are defined for this example:
 * - In receiver role, the microapp listens for incoming mesh messages
 * - In transmitter role, the microapp periodically transmits mesh messages
 */

// Define or undefine the following to set the role for the crownstone
// #define ROLE_RECEIVER
#define ROLE_TRANSMITTER

uint32_t counter;

void printMeshMsg(MeshMsg* msg) {
	Serial.print("Received mesh message from stone ");
	Serial.println(msg->stoneId);
	Serial.println(msg->dataPtr, msg->size);
}

void meshCallback(MeshMsg msg) {
	printMeshMsg(&msg);
}

void setup() {
	Serial.begin();
	// Write something to the log (will be shown in the bluenet code as print statement).
	Serial.println("Hello world example");

	counter = 0;

#ifdef ROLE_RECEIVER
	Serial.println("Start listening to mesh");
	if (!Mesh.listen()) {
		Serial.println("Mesh.listen() failed");
	}
	Mesh.setIncomingMeshMsgHandler(meshCallback);

#endif

	short id = Mesh.id();
	Serial.print("Own stone id is ");
	Serial.println(id);
}

void loop() {
#ifdef ROLE_RECEIVER
	// Read Mesh
	if (Mesh.available()) {
		MeshMsg msg;
		Mesh.readMeshMsg(&msg);
		printMeshMsg(&msg);
	}
#endif
#ifdef ROLE_TRANSMITTER
	if (counter % 10 == 0) {
		// Send Mesh
		Serial.println("Sending mesh msg");
		uint8_t msg[2] = {0xAB, 0xCD};
		// use 0 as stoneId for broadcast
		uint8_t stoneId = 0;
		Mesh.sendMeshMsg(msg, sizeof(msg), stoneId);
	}
#endif
	counter++;
}

