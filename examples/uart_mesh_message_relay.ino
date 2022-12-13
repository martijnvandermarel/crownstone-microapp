#include <Arduino.h>
#include <Message.h>
#include <Mesh.h>

// This app is intended to be run on a crownstone USB dongle
// This app fulfills two functions:
// - Listens for microapp mesh messages and upon receiving one forwards it over uart
// - Listens for microapp uart messages and upon receiving one forwards it over the mesh

// Only microapp messages with this opcode are forwarded over the mesh
const uint8_t MICROAPP_MESSAGE_MESH_FORWARD_OPCODE = 0x78;

// opcode (0x78) + stone id + mesh message payload
uint8_t uartMsgBuf[1 + 1 + MAX_MICROAPP_MESH_PAYLOAD_SIZE];
uint8_t uartPayloadSize = 0;

uint8_t meshMsgBuf[MAX_MICROAPP_MESH_PAYLOAD_SIZE];
uint8_t meshReceiverId = 0;
uint8_t meshPayloadSize = 0;

bool sendMeshFlag = false;
bool sendUartFlag = false;

void onMeshMessage(MeshMsg msg) {
	if (msg.size > MAX_MICROAPP_MESH_PAYLOAD_SIZE) {
		msg.size = MAX_MICROAPP_MESH_PAYLOAD_SIZE;
	}
	uartMsgBuf[0] = MICROAPP_MESSAGE_MESH_FORWARD_OPCODE;
	uartMsgBuf[1] = msg.stoneId;
	memcpy(uartMsgBuf + 2, msg.dataPtr, msg.size);
	uartPayloadSize = msg.size + 2;
	sendUartFlag = true;
}

void onUartMessage(uint8_t* data, microapp_size_t size) {
	uint8_t opcode = data[0];
	if (opcode != MICROAPP_MESSAGE_MESH_FORWARD_OPCODE) {
		return;
	}
	meshPayloadSize = size - 2;
	if (meshPayloadSize > MAX_MICROAPP_MESH_PAYLOAD_SIZE) {
		meshPayloadSize = MAX_MICROAPP_MESH_PAYLOAD_SIZE;
	}
	meshReceiverId = data[1];
	memcpy(meshMsgBuf, data + 2, meshPayloadSize);
	sendMeshFlag = true;
}

void setup() {
	Serial.begin();
	Serial.println("UART <-> Mesh relay");

	if (!Message.begin()) {
		Serial.println("Message.begin() failed");
	}
	Message.setHandler(onUartMessage);

	if (!Mesh.listen()) {
		Serial.println("Mesh.listen() failed");
	}
	Mesh.setIncomingMeshMsgHandler(onMeshMessage);

}

void loop() {
	if (sendMeshFlag) {
		Serial.print("Sending message over mesh to ");
		Serial.println(meshReceiverId);
		Serial.println(meshMsgBuf, meshPayloadSize);
		Mesh.sendMeshMsg(meshMsgBuf, meshPayloadSize, meshReceiverId);
		sendMeshFlag = false;
	}
	if (sendUartFlag) {
		Serial.println("Sending message over uart");
		Serial.println(uartMsgBuf, uartPayloadSize);
		Message.write(uartMsgBuf, uartPayloadSize);
		sendUartFlag = false;
	}

}
