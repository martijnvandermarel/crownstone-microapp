#include <Arduino.h>
#include <PowerUsage.h>
#include <ArduinoBLE.h>

uint32_t counter = 0;

// Has to be less than MAX_CHARACTERISTIC_VALUE_SIZE / sizeof(uint32_t)
static const uint8_t BUFFER_SIZE = 20;
static uint32_t buffer[BUFFER_SIZE];
// The first element of the buffer is an index indicating the newest element of the buffer.
static uint32_t& newestDataIndex = buffer[0];
// The second element of the buffer indicates the number of available bytes in the buffer
static uint32_t& nrAvailableEntries = buffer[1];
// The rest of the buffer is dedicated to the power data
static uint32_t* powerBuffer = &buffer[2];
static const uint8_t POWER_BUFFER_SIZE = BUFFER_SIZE - 2;

BleCharacteristic readPowerBufferCharacteristic;
BleCharacteristic clearPowerBufferCharacteristic;
BleService powerService;

void addToPowerBuffer(uint32_t entry) {
	// if no entries are available, the (first) entry will be at index 0, so don't increment
	if (nrAvailableEntries != 0) {
		newestDataIndex++;
	}
	// if the new data index is out of range, overflow back to 0
	if (newestDataIndex >= POWER_BUFFER_SIZE) {
		newestDataIndex = 0;
	}
	// as long as we don't overflow, the number of available bytes increases
	else {
		nrAvailableEntries++;
	}
	powerBuffer[newestDataIndex] = entry;
}

void clearPowerBuffer() {
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
		powerBuffer[i] = 0;
	}
	newestDataIndex = 0;
	nrAvailableEntries = 0;
}

void setup() {
	Serial.begin();
	Serial.println("High frequency power peripheral");

	if (!BLE.begin()) {
		Serial.println("BLE.begin failed");
		return;
	}

	// Service with random uuid. The middle 2 parts can be used as respectively COMPANY - LOCATION identifiers
	powerService = BleService("95375502-0001-0001-0000-3AA5B130D4E7");
	// In characteristic uuid, use last 2-byte part to identify characteristic within the service
	readPowerBufferCharacteristic = BleCharacteristic("95375502-0001-0001-0001-3AA5B130D4E7",
		BleCharacteristicProperties::BLERead, (uint8_t*)buffer, sizeof(buffer));

	powerService.addCharacteristic(readPowerBufferCharacteristic);
	BLE.addService(powerService);
}

void loop() {
	if (clearPowerBufferCharacteristic.written()) {
		clearPowerBuffer();
	}

	addToPowerBuffer(++counter);

	// Make sure the loop is called at about once per second.
	delay(1000);
}
