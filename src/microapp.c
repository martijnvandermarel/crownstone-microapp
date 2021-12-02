#include <microapp.h>
#include <ipc/cs_IpcRamData.h>

// Important: Do not include <string.h> / <cstring>. This bloats the binary unnecessary.
// On Arduino there is the String class. Roll your own functions like strlen, see below.

// returns size MAX_PAYLOAD for strings that are too long, note that this can still not fit in the payload
// the actually supported string length depends on the opcode
// the limit here is just to prevent looping forever
uint8_t strlen(const char *str) {
	for (uint8_t i = 0; i < MAX_PAYLOAD; ++i) {
		if (str[i] == 0) {
			return i;
		}
	}
	return MAX_PAYLOAD;
}

// compares two buffers of length len, bufA and bufB
// returns 0 if bufA and bufB are equal
// returns -1 if for the first unmatching byte i we have bufA[i] < bufB[i]
// returns 1 if for the first unmatching byte i we have bufA[i] > bufB[i]
int memcmp(const void *bufA, const void *bufB, size_t len) {
	char *p = (char*) bufA;
	char *q = (char*) bufB;
	if (bufA == bufB) { // point to the same address
		return 0;
	}
	for (uint8_t i = 0; i<len; i++) {
		if (*(p+i) < *(q+i)) {
			return -1;
		}
		else if (*(p+i) > *(q+i)) {
			return 1;
		}
	}
	return 0;
}

char* strcpy(char* dest, const char* src) {
	char* p = (char*) src;
	for (uint8_t i = 0; i < MAX_PAYLOAD; i++) {
		*(dest+i) = *p;
		if (*p == 0) {
			break;
		}
		p++;
	}
	return dest;
}

/*
 * A global object to send a message.
 */
microapp_message_t global_msg;

/*
 * Send the actual message.
 */
int sendMessage(microapp_message_t *msg) {
	int result = -1;

	// Check length.
	if (msg->length > MAX_PAYLOAD) {
		return result;
	}

	// QUESTION: can we cache the callback function, so we don't have to get it from ipc ram data every time?

	// Clear buffer.
	// QUESTION: why does it have to be set to zero?
	uint8_t buf[BLUENET_IPC_RAM_DATA_ITEM_SIZE];
	for (int i = 0; i < BLUENET_IPC_RAM_DATA_ITEM_SIZE; ++i) {
		buf[i] = 0;
	}

	// Write buffer with ram data.
	uint8_t rd_size = 0;
	uint8_t ret_code = getRamData(IPC_INDEX_CROWNSTONE_APP, buf, BLUENET_IPC_RAM_DATA_ITEM_SIZE, &rd_size);

	// Check if the (right) struct exists.
	// QUESTION: can we do this check before the getRamData call?
	bluenet_ipc_ram_data_item_t *ramStruct = getRamStruct(IPC_INDEX_MICROAPP);
	if (!ramStruct) {
		return result;
	}

	// Obtain callback function from the ram data.
	uintptr_t _callback = 0;
	if (ret_code == IPC_RET_SUCCESS) {
		uint8_t protocol = buf[0];
		// Only accept protocol version 0.
		if (protocol == 0) {
			uint8_t offset = 1;
			for (int i = 0; i < 4; ++i) {
				_callback = _callback | ( (uintptr_t)(buf[i+offset]) << (i*8));
			}
		}
	}

	// The actual callback.
	if (_callback) {
		int (*callback_func)(char*,uint16_t) = (int (*)(char*,uint16_t)) _callback;
		result = callback_func((char*)msg->payload, msg->length);
	}
	return result;
}
