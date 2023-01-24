#include <Arduino.h>
#include <ArduinoBLE.h>
#include <Mesh.h>

// const char* beaconAddress = "A4:C1:38:9A:45:E3";
const char* beaconAddress = "A4:C1:38:6A:69:57";
// const char* beaconName = "ATC_9A45E3";
const char* beaconName = "ATC_6A6957";

bool available = false;

const uint8_t APP_IDENTIFIER = 0xFF;
uint8_t meshMsgBuf[5];

// callback for received peripheral advertisement
void onScan(BleDevice& device) {
	// parse service data of beacon advertisement if available
	ble_ad_t serviceData;
	if (device.findAdvertisementDataType(GapAdvType::ServiceData16BitUuid, &serviceData)) {
		if (serviceData.len == 15) { // service data length of the ATC service data advertisements
			// copy temperature, humidity and battery level bytes to mesh msg buffer
			memcpy(&(meshMsgBuf[1]), &(serviceData.data[8]), 4);
			available = true;
		}
	}
}

// The Arduino setup function.
void setup() {
	Serial.println("Temperature sensor");

	// Start BLE device
	if (!BLE.begin()) {
		return;
	}

	// Register scan handler
	if (!BLE.setEventHandler(BLEDeviceScanned, onScan)) {
		return;
	}

	// set app identifier
	meshMsgBuf[0] = APP_IDENTIFIER;

	Serial.println("Successfully finished setup");
}

// The Arduino loop function.
void loop() {
	BLE.scanForAddress(beaconAddress);
	uint16_t attempts = 100; // in units of 100ms
	while (!available) {
		delay(100);
		if (attempts-- == 0) {
			break;
		}
	}
	if (available) {
		// receiver id is 0 (=broadcast) but can also be id of the dongle
		uint8_t receiverStoneId = 13;
		Mesh.sendMeshMsg(meshMsgBuf, sizeof(meshMsgBuf), receiverStoneId);
		Serial.println(meshMsgBuf, sizeof(meshMsgBuf));
	}
	// stop scanning
	BLE.stopScan();
	// reset flag
	available = false;
	// sleep for a minute
	delay(60000);
}
