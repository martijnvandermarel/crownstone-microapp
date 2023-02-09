#include <Arduino.h>
#include <ArduinoBLE.h>

int32_t counter = 0;

// readable buffer characteristic
// Has to be less than MAX_CHARACTERISTIC_VALUE_SIZE
static const uint8_t BUFFER_SIZE = 203;
static uint8_t buffer[BUFFER_SIZE];
// The first byte of the buffer is an index indicating the newest element of the buffer.
static uint8_t& newestEntryIndex = buffer[0];
// The second byte of the buffer indicates the number of available bytes in the buffer
static uint8_t& nrAvailableEntries = buffer[1];
// The third byte of the buffer indicates the maximum number of bytes in the buffer.
static uint8_t& maxEntries = buffer[2];
// The rest of the buffer is dedicated to the power data
static int32_t* powerEntries = (int32_t*)&buffer[3];
static uint8_t HEADER_SIZE = sizeof(newestEntryIndex) + sizeof(nrAvailableEntries) + sizeof(maxEntries);
static const uint8_t MAX_POWER_ENTRIES = (BUFFER_SIZE - HEADER_SIZE) / sizeof(int32_t);

// clear characteristic
static uint8_t clearBufVal[1] = {0};

// sampling frequency characteristic
// The characteristic readable and writable buffer
static uint8_t samplingFrequencyVal[1] = {10}; // default is 1 second or 10*100ms
static uint8_t samplingFrequency;

BleCharacteristic readPowerBufferCharacteristic;
BleCharacteristic clearPowerBufferCharacteristic;
BleCharacteristic samplingFrequencyCharacteristic;
BleService powerService;

void addPowerEntry(int32_t entry) {
	// if no entries are available, the (first) entry will be at index 0, so don't increment
	if (nrAvailableEntries != 0) {
		newestEntryIndex++;
	}
	// if the new data index is out of range, overflow back to 0
	if (newestEntryIndex >= MAX_POWER_ENTRIES) {
		newestEntryIndex = 0;
	}
	// as long as we don't overflow, the number of available bytes increases
	if (nrAvailableEntries < MAX_POWER_ENTRIES) {
		nrAvailableEntries++;
	}
	powerEntries[newestEntryIndex] = entry;
}

void clearBuffer() {
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
		buffer[i] = 0;
	}
	maxEntries = MAX_POWER_ENTRIES;
}

void setup() {
	Serial.begin();
	Serial.println("High frequency power peripheral");

	if (!BLE.begin()) {
		Serial.println("BLE.begin failed");
		return;
	}

	clearBuffer();

	// Service with random uuid. The middle parts can be used as identifiers for e.g. company or location
	powerService = BleService("95370000-0001-0001-0001-3AA5B130D4E7");
	readPowerBufferCharacteristic = BleCharacteristic("95370001-0001-0001-0001-3AA5B130D4E7",
		BleCharacteristicProperties::BLERead, buffer, sizeof(buffer));
	clearPowerBufferCharacteristic = BleCharacteristic("95370002-0001-0001-0001-3AA5B130D4E7",
		BleCharacteristicProperties::BLEWrite, clearBufVal, sizeof(clearBufVal));
	samplingFrequencyCharacteristic = BleCharacteristic("95370003-0001-0001-0001-3AA5B130D4E7",
		BleCharacteristicProperties::BLEWrite | BleCharacteristicProperties::BLERead,
		samplingFrequencyVal, sizeof(samplingFrequencyVal));

	powerService.addCharacteristic(readPowerBufferCharacteristic);
	powerService.addCharacteristic(clearPowerBufferCharacteristic);
	powerService.addCharacteristic(samplingFrequencyCharacteristic);
	BLE.addService(powerService);

	// write the initial sampling frequency to the characteristic
	samplingFrequency = *samplingFrequencyVal;
	samplingFrequencyCharacteristic.writeValue(samplingFrequencyVal, sizeof(samplingFrequencyVal));
}

void loop() {
	if (clearPowerBufferCharacteristic.written()) {
		clearBuffer();
	}
	if (samplingFrequencyCharacteristic.written()) {
		if (samplingFrequency != *samplingFrequencyCharacteristic.value()) {
			samplingFrequency = *samplingFrequencyCharacteristic.value();
			// also clear buffer since it will be weird to have changed sampling frequency halfway through the buffer
			// plus sampling frequency byte in header will be overwritten with the new value
			clearBuffer();
		}
	}

	addPowerEntry(++counter);
	uint8_t availableBytes = (nrAvailableEntries * sizeof(int32_t)) + HEADER_SIZE;
	readPowerBufferCharacteristic.writeValue(buffer, availableBytes);

	BleDevice& central = BLE.central();
	if (central) {
		// Keep the connection alive
		central.connectionKeepAlive();
	}

	delay(100 * samplingFrequency);
}
