#include <BleScan.h>

BleScan::BleScan(uint8_t* scanData, uint8_t scanSize) {
	if (scanSize > MAX_BLE_ADV_DATA_LENGTH) {
		scanSize = MAX_BLE_ADV_DATA_LENGTH;
	}
	_scanData = scanData;
	_scanSize = scanSize;
}

ble_ad_t BleScan::localName() {
	ble_ad_t localName;
	if (findAdvertisementDataType(GapAdvType::CompleteLocalName, &localName)) {
		return localName; // filled
	}
	else if (findAdvertisementDataType(GapAdvType::ShortenedLocalName, &localName)) {
		return localName; // filled
	}
	else {
		return localName; // empty
	}
}

bool BleScan::findAdvertisementDataType(GapAdvType type, ble_ad_t* foundData) {
	uint8_t i       = 0;
	foundData->type = 0;
	foundData->data = nullptr;
	foundData->len  = 0;
	while (i < _scanSize - 1) {
		uint8_t fieldLen  = _scanData[i];
		uint8_t fieldType = _scanData[i + 1];
		if (fieldLen == 0 || i + 1 + fieldLen > _scanSize) {
			return false;
		}
		if (fieldType == type) {
			foundData->data = &_scanData[i + 2];
			foundData->len  = fieldLen - 1;
			foundData->type = (uint8_t)type;
			return true;
		}
		i += fieldLen + 1;
	}
	return false;
}

bool BleScan::hasServiceDataUuid(uuid16_t uuid) {
	GapAdvType serviceUuidListTypes[2] = {
			GapAdvType::IncompleteList16BitServiceUuids,
			GapAdvType::CompleteList16BitServiceUuids};
	ble_ad_t ad;
	for (uint8_t i = 0; i < sizeof(serviceUuidListTypes)/sizeof(serviceUuidListTypes[0]); i++) {
		if (findAdvertisementDataType(serviceUuidListTypes[i], &ad)) {
			// check ad for uuid
			for (uint8_t j = 0; j < ad.len; j += sizeof(uuid)) {
				if (uuid == ((ad.data[j + 1] << 8) | ad.data[j])) {
					return true;
				}
			}
		}
	}
	return false;
}