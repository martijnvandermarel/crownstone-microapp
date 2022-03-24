#include <ArduinoBLE.h>
#include <Mesh.h>

// A ble microapp example for sending/receiving rssi data over the mesh.

// MAC address of the asset to be tracked
const char* assetAddress = "A4:C1:38:9A:45:E3";

// Crownstone can either be a normal node (not connected over UART) or act as relayer (connected to a device over UART)
enum Role {
	NODE         = 0,
	UART_RELAYER = 1,
} role;

// Crownstone ID of this crownstone and that of the crownstone acting as relay
uint8_t thisCrownstoneId = 6;
uint8_t relayCrownstoneId = 6;

// identifier for mesh messages to identify those sending rssi values
uint8_t rssiUuid = 0x99;

// initialize microapp service data
uint8_t serviceDataBuf[12] = {0};

// initialize mesh
Mesh mesh;

// callback for received peripheral advertisement
void onAssetScan(BleDevice device) {
	Serial.println("my_callback_func: ");
	Serial.print("\trssi: "); Serial.println(device.rssi());
	Serial.print("\taddress: "); Serial.println(device.address().c_str());

	if (role == Role::NODE) {
		// send over mesh
		uint8_t urssi = (uint8_t) device.rssi();
		uint8_t msg[2] = {rssiUuid, urssi};
		mesh.sendMeshMsg(msg, sizeof(msg), relayCrownstoneId);
	}
	else if (role == Role::UART_RELAYER) {
		// send over uart
		serviceDataBuf[2] = thisCrownstoneId;
		serviceDataBuf[3] = device.rssi();
		SerialServiceData.write(serviceDataBuf, sizeof(serviceDataBuf));
	}
}


// The Arduino setup function.
void setup() {
	// We can "start" serial.
	Serial.begin();

	// Write something to the log (will be shown in the bluenet code as print statement).
	Serial.println("Setup");

	// Set the UUID of this microapp.
	if (thisCrownstoneId == relayCrownstoneId) {
		role = Role::UART_RELAYER;
		serviceDataBuf[0] = 0;
		serviceDataBuf[1] = rssiUuid; // microapp uuid
	}
	else {
		role = Role::NODE;
	}

	// Register callback for scan of asset
	BLE.setEventHandler(BleEventDeviceScanned, onAssetScan);

	BLE.scanForAddress(assetAddress);
}

// The Arduino loop function.
void loop() {
	// Say something every time we loop (which is every second)
	Serial.println("Loop");

	if (role == Role::UART_RELAYER) {
		if (mesh.available()) {
			uint8_t* msg_ptr = nullptr;
			uint8_t senderId = 0;
			uint8_t size = mesh.readMeshMsg(&msg_ptr, &senderId);

			Serial.print("Mesh message from id "); Serial.println(senderId);
			Serial.println(msg_ptr, size);

			if (msg_ptr[0] == rssiUuid) {
				Serial.println("rssiUuid match");
				serviceDataBuf[2] = senderId;
				serviceDataBuf[3] = msg_ptr[1];
				SerialServiceData.write(serviceDataBuf, sizeof(serviceDataBuf));
			}

		}
	}
	return;
}