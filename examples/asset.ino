#include <Arduino.h>
#include <BleMacAddress.h>

microapp_sdk_result_t onAssetEvent(microapp_sdk_asset_t* asset) {
	Serial.println("Asset!");
	uint32_t assetId = asset->event.assetId[0] | ((asset->event.assetId[1] << 8) & 0xFF00) | ((asset->event.assetId[2] << 16) & 0xFF0000);
	Serial.println((unsigned int) assetId);
	Serial.println((int)asset->event.rssi);
	Serial.println(MacAddress(asset->event.address.address, MAC_ADDRESS_LENGTH, asset->event.address.type).string());
	return CS_MICROAPP_SDK_ACK_SUCCESS;
}

void setup() {
	Serial.println("Asset example");

	interrupt_registration_t interrupt;
	interrupt.type          = CS_MICROAPP_SDK_TYPE_ASSETS;
	interrupt.id            = CS_MICROAPP_SDK_ASSET_EVENT;
	interrupt.handler        = reinterpret_cast<interruptFunction>(onAssetEvent);
	microapp_sdk_result_t result = registerInterrupt(&interrupt);
	if (result != CS_MICROAPP_SDK_ACK_SUCCESS) {
		Serial.println("Registering microapp interrupt failed");
		return;
	}

	uint8_t* payload = getOutgoingMessagePayload();
	microapp_sdk_asset_t* request = reinterpret_cast<microapp_sdk_asset_t*>(payload);
	request->header.messageType = CS_MICROAPP_SDK_TYPE_ASSETS;
	request->header.ack = CS_MICROAPP_SDK_ACK_REQUEST;
	request->type = CS_MICROAPP_SDK_ASSET_REGISTER_INTERRUPT;
	sendMessage();
}

void loop() {

}
