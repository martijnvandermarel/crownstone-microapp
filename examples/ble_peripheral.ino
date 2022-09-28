#include <Arduino.h>
#include <ArduinoBLE.h>

/**
 * A microapp example for acting as a BLE peripheral
 */

// This is the value to be used in the temperature characteristic
float temperatureCelsius = 22.5;
static const uint8_t NR_TEMPERATURE_BYTES = 1;
uint8_t temperatureValue[NR_TEMPERATURE_BYTES] = {0x7F};
// See Bluetooth SIG assigned numbers for the 16-bit UUIDs
// 0x181A: Environmental Sensing
BleService temperatureService("181A");
// 0x2B0D: Temperature 8
BleCharacteristic temperatureCharacteristic("2B0D", BleCharacteristicProperties::BLERead,
	temperatureValue, NR_TEMPERATURE_BYTES);


void updateTemperature() {
	// Simulate some temperature changes
	temperatureCelsius += 0.5;
	if (temperatureCelsius > 38.5) {
		temperatureCelsius = -7.5;
	}
	// See GATT Specification Supplement by Bluetooth SIG
	int8_t temperature8 = (int8_t)temperatureCelsius * 2;
	temperatureValue[0] = temperature8;
	temperatureCharacteristic.writeValue(temperatureValue, NR_TEMPERATURE_BYTES);
}

void onCentralConnected(BleDevice device) {
	Serial.println("BLE central connected");
}

void onCentralDisconnected(BleDevice device) {
	Serial.println("BLE central disconnected");
}

// The Arduino setup function.
void setup() {
	Serial.begin();

	// Write something to the log (will be shown in the bluenet code as print statement).
	Serial.println("BLE peripheral example");

	if (!BLE.begin()) {
		Serial.println("BLE.begin failed");
		return;
	}

	// Register scan handler
	if (!BLE.setEventHandler(BLEConnected, onCentralConnected)) {
		Serial.println("Setting event handler failed");
		return;
	}
	if (!BLE.setEventHandler(BLEDisconnected, onCentralDisconnected)) {
		Serial.println("Setting event handler failed");
		return;
	}
	temperatureService.addCharacteristic(temperatureCharacteristic);
	BLE.addService(temperatureService);
	temperatureCharacteristic.writeValue(temperatureValue, NR_TEMPERATURE_BYTES);
}

// The Arduino loop function.
void loop() {
	updateTemperature();

	// central returns a reference. Making copies can break stuff
	BleDevice& central = BLE.central();

	if (central) {
		Serial.println("Central device connected");
		Serial.println(central.address());
	}
}
