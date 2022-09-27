#pragma once

#include <microapp.h>

// Incomplete list of GAP advertisement types, see
// https://www.bluetooth.com/specifications/assigned-numbers/
enum GapAdvType {
	Flags                            = 0x01,
	IncompleteList16BitServiceUuids  = 0x02,
	CompleteList16BitServiceUuids    = 0x03,
	IncompleteList32BitServiceUuids  = 0x04,
	CompleteList32BitServiceUuids    = 0x05,
	IncompleteList128BitServiceUuids = 0x06,
	CompleteList128BitServiceUuids   = 0x07,
	ShortenedLocalName               = 0x08,
	CompleteLocalName                = 0x09,
	ServiceData16BitUuid             = 0x16,
	ServiceData32BitUuid             = 0x20,
	ServiceData128BitUuid            = 0x21,
	ManufacturerSpecificData         = 0xFF,
};

// Types of BLE event for which event handlers can be set
// The naming of these corresponds with ArduinoBLE syntax
enum BleEventType {
	// BleDevice
	BLEDeviceScanned = 0x01,
	BLEConnected     = 0x02,
	BLEDisconnected  = 0x03,
	// BleCharacteristic
	BLESubscribed   = 0x04,
	BLEUnsubscribed = 0x05,
	BLERead         = 0x06,
	BLEWritten      = 0x07,
};

// Forward declarations
class BleDevice;
class BleCharacteristic;

typedef void (*DeviceEventHandler)(BleDevice);
typedef void (*CharacteristicEventHandler)(BleDevice, BleCharacteristic);
// Both of the above can be cast to a  (generic) BleEventHandler (and back)
// so both can be stored in the BleEventHandlerRegistration
typedef void (*BleEventHandler)(void);

// Registration for user callbacks that can be kept local.
struct BleEventHandlerRegistration {
	BleEventHandler eventHandler = nullptr;
	bool filled                  = false;
	BleEventType eventType;
};

typedef int8_t rssi_t;

/**
 * Convert a pair of chars to a byte, e.g. convert "A3" to 0xA3.
 *
 * @param[in] chars   Pointer to a pair of chars to convert to a byte.
 *
 * @return            The byte as a uint8_t.
 */
uint8_t convertTwoHexCharsToByte(const char* chars);

/**
 * Convert a byte (uint8_t) to its hex string representation, e.g. convert 0xA3 to "A3".
 *
 * @param[in] byte   Byte to be converted to a pair of chars.
 * @param[out] res   Pointer to a pair of chars.
 */
void convertByteToTwoHexChars(uint8_t byte, char* res);
