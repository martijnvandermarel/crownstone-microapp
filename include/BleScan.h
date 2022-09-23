#pragma once

#include <BleUtils.h>
#include <String.h>
#include <microapp.h>

struct ble_ad_t {
	uint8_t type  = 0;
	uint8_t len   = 0;
	uint8_t* data = nullptr;
};

/**
 * Helper wrapper class for scan data
 * Does not contain the actual data but only a pointer to it
 */
class BleScan {
private:
	friend class BleDevice;
	friend class Ble;

	BleScan(){}; // default constructor

	uint8_t* _scanData = nullptr;
	uint8_t _scanSize = 0;

	/**
	 * Tries to find an ad of specified GAP ad data type. and if found returns true and a pointer to its location.
	 *
	 * @param[in] type          GAP advertisement type
	 * @param[out] foundData    ad containing a pointer to data and its length
	 *
	 * @return true             if the advertisement data of given type is found.
	 * @return false            if the advertisement data of given type is not found.
	 */
	bool findAdvertisementDataType(GapAdvType type, ble_ad_t* foundData);

public:
	BleScan(uint8_t* scanData, uint8_t scanSize);

	/**
	 * Query the local name advertised in the scan (either complete or shortened)
	 *
	 * @return An ad with a pointer to the local name and its length (nullptr and 0 if not found, respectively)
	 */
	ble_ad_t localName();

	/**
	 * Checks if a service with passed uuid is advertised by the device
	 *
	 * @param uuid     the uuid to filter
	 * @return true    if found
	 * @return false   if not found
	 */
	bool hasServiceDataUuid(uuid16_t uuid);

};