#include <ipc/cs_IpcRamData.h>
#include <microapp.h>

// Define array with soft interrupts
interrupt_registration_t interruptRegistrations[MAX_INTERRUPT_REGISTRATIONS];

// Incremental version apart from IPC struct
const uint8_t MICROAPP_IPC_CURRENT_PROTOCOL_VERSION           = 1;

// Important: Do not include <string.h> / <cstring>. This bloats up the binary unnecessary.
// On Arduino there is the String class. Roll your own functions like strlen, see below.

// returns size MAX_STRING_SIZE for strings that are too long, note that this can still not fit in the payload
// the actually supported string length depends on the opcode
// the limit here is just to prevent looping forever
uint8_t strlen(const char* str) {
	for (microapp_size_t i = 0; i < MAX_STRING_SIZE; ++i) {
		if (str[i] == 0) {
			return i;
		}
	}
	return MAX_STRING_SIZE;
}

// compares two buffers of length num, ptr1 and ptr2
// returns 0 if ptr1 and ptr2 are equal
// returns -1 if for the first unmatching byte i we have ptr1[i] < ptr2[i]
// returns 1 if for the first unmatching byte i we have ptr1[i] > ptr2[i]
int memcmp(const void* ptr1, const void* ptr2, microapp_size_t num) {
	char* p = (char*)ptr1;
	char* q = (char*)ptr2;
	if (ptr1 == ptr2) {  // point to the same address
		return 0;
	}
	for (microapp_size_t i = 0; i < num; ++i) {
		if (*(p + i) < *(q + i)) {
			return -1;
		}
		else if (*(p + i) > *(q + i)) {
			return 1;
		}
	}
	return 0;
}

void* memcpy(void* dest, const void* src, microapp_size_t num) {
	uint8_t* p = (uint8_t*)src;
	uint8_t* q = (uint8_t*)dest;
	for (microapp_size_t i = 0; i < num; ++i) {
		*q = *p;
		p++;
		q++;
	}
	return dest;
}

/*
 * A global object for messages in and out.
 * Accessible by both bluenet and the microapp
 */
static bluenet_io_buffers_t shared_io_buffers;

/*
 * A global object for ipc data as well.
 */
static bluenet_ipc_data_cpp_t ipc_data;

uint8_t* getOutgoingMessagePayload() {
	return shared_io_buffers.microapp2bluenet.payload;
}

uint8_t* getIncomingMessagePayload() {
	return shared_io_buffers.bluenet2microapp.payload;
}

/*
 * Struct that stores copies of the shared io buffer
 */
struct stack_entry_t {
	bluenet_io_buffers_t ioBuffer;
	bool filled;
};

/*
 * Defines how 'deep' nested interrupts can go.
 * For each level, an interrupt buffer and a request buffer are needed to store the state of the level above.
 */
static const uint8_t MAX_INTERRUPT_DEPTH = 3;

/*
 * Stack of io buffer copies. For each interrupt layer, the stack grows
 */
static stack_entry_t stack[MAX_INTERRUPT_DEPTH];

static bool stack_initialized = false;

// Cache whether the IPC ram data from bluenet is valid.
static bool ipcValid = false;

/*
 * Function checkRamData is used in sendMessage.
 */
microapp_sdk_result_t checkRamData(bool checkOnce) {
	if (!stack_initialized) {
		for (int8_t i = 0; i < MAX_INTERRUPT_DEPTH; ++i) {
			stack[i].filled = false;
		}
		stack_initialized = true;
	}

	if (checkOnce) {
		// If valid is set, we assume cached values are fine, otherwise load them.
		if (ipcValid) {
			return CS_MICROAPP_SDK_ACK_SUCCESS;
		}
	}

	uint8_t dataSize;
	uint8_t retCode = getRamData(IPC_INDEX_BLUENET_TO_MICROAPP, ipc_data.raw, &dataSize, sizeof(ipc_data.raw));

	if (retCode != 0) {
		return CS_MICROAPP_SDK_ACK_ERROR;
	}

	if (dataSize != sizeof(bluenet2microapp_ipcdata_t)) {
		return CS_MICROAPP_SDK_ACK_ERROR;
	}

	if (!ipc_data.bluenet2microappData.microappCallback) {
		return CS_MICROAPP_SDK_ACK_ERROR;
	}

	// Check protocol.
	if (ipc_data.bluenet2microappData.dataProtocol != MICROAPP_IPC_DATA_PROTOCOL) {
		return CS_MICROAPP_SDK_ACK_ERROR;
	}

	ipcValid = true;
	microapp_sdk_result_t result = CS_MICROAPP_SDK_ACK_SUCCESS;

	if (checkOnce) {
		// Write the buffer only once
		microappCallbackFunc callbackFunctionIntoBluenet = ipc_data.bluenet2microappData.microappCallback;
		result = callbackFunctionIntoBluenet(CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER, &shared_io_buffers);
	}
	return result;
}

/*
 * Returns the number of empty slots for bluenet.
 */
uint8_t emptySlotsInStack() {
	uint8_t totalEmpty = 0;
	for (uint8_t i = 0; i < MAX_INTERRUPT_DEPTH; ++i) {
		if (!stack[i].filled) {
			totalEmpty++;
		}
	}
	return totalEmpty;
}

int8_t getNextEmptyStackSlot() {
	for (uint8_t i = 0; i < MAX_INTERRUPT_DEPTH; ++i) {
		if (!stack[i].filled) {
			return i;
		}
	}
	return -1;
}

/*
 * Handle incoming interrupts from bluenet
 */
void handleBluenetInterrupt() {
	uint8_t* incomingPayload              = getIncomingMessagePayload();
	microapp_sdk_header_t* incomingHeader = reinterpret_cast<microapp_sdk_header_t*>(incomingPayload);
	// First check if this is not just a regular call
	if (incomingHeader->ack != CS_MICROAPP_SDK_ACK_REQUEST) {
		// No request, so this is not an interrupt
		return;
	}
	// Check if we have the capacity to handle another interrupt
	int8_t emptySlots = emptySlotsInStack();
	if (emptySlots == 0) {
		// Max depth has been reached, drop the interrupt and return
		incomingHeader->ack = CS_MICROAPP_SDK_ACK_ERR_BUSY;
		// Yield to bluenet, without writing in the outgoing buffer.
		// Bluenet will check the written ack field
		sendMessage();
		return;
	}
	// Get the index of the first empty stack slot
	int8_t stackIndex = getNextEmptyStackSlot();
	if (stackIndex < 0) {
		// Apparently there was no space. Should not happen since we just checked
		// In any case, let's just drop and return similarly to above
		incomingHeader->ack = CS_MICROAPP_SDK_ACK_ERR_BUSY;
		sendMessage();
		return;
	}
	// Copy the shared buffers to the top of the stack
	stack_entry_t* newStackEntry = &stack[stackIndex];
	uint8_t* outgoingPayload     = getOutgoingMessagePayload();
	// Copying the outgoing buffer is needed so that the outgoing buffer can be freely used
	// for microapp requests during the interrupt handling
	memcpy(newStackEntry->ioBuffer.microapp2bluenet.payload, outgoingPayload, MICROAPP_SDK_MAX_PAYLOAD);
	// Copying the incoming buffer is needed so that the interrupt payload is preserved
	// if bluenet generates another interrupt before finishing handling this one
	memcpy(newStackEntry->ioBuffer.bluenet2microapp.payload, incomingPayload, MICROAPP_SDK_MAX_PAYLOAD);
	newStackEntry->filled = true;

	// Mark the incoming ack as 'in progress' so bluenet will keep calling
	incomingHeader->ack = CS_MICROAPP_SDK_ACK_IN_PROGRESS;

	// Now the interrupt will actually be handled
	// Call the interrupt handler and pass a pointer to the copy of the interrupt buffer
	// Note that new sendMessage calls may occur in the interrupt handler
	microapp_sdk_header_t* stackEntryHeader =
			reinterpret_cast<microapp_sdk_header_t*>(newStackEntry->ioBuffer.bluenet2microapp.payload);
	microapp_sdk_result_t result = handleInterrupt(stackEntryHeader);

	// When done with the interrupt handling, we can pop the buffers from the stack again
	// Though really we only need the outgoing buffer, since we just finished dealing with the incoming buffer
	memcpy(outgoingPayload, newStackEntry->ioBuffer.microapp2bluenet.payload, MICROAPP_SDK_MAX_PAYLOAD);
	newStackEntry->filled = false;

	// End with a sendMessage call which yields back to bluenet
	// Bluenet will see the acknowledge and not call again
	incomingHeader->ack = result;
	sendMessage();
	return;
}

/*
 * Send the actual message to bluenet
 *
 * If there are no interrupts it will just return and at some later time be called again.
 */
microapp_sdk_result_t sendMessage() {
	bool checkOnce           = true;
	microapp_sdk_result_t result = checkRamData(checkOnce);
	if (result != CS_MICROAPP_SDK_ACK_SUCCESS) {
		return result;
	}

	// The callback will yield control to bluenet.
	microappCallbackFunc callbackFunctionIntoBluenet = ipc_data.bluenet2microappData.microappCallback;
	uint8_t opcode = checkOnce ? CS_MICROAPP_CALLBACK_SIGNAL : CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER;
	result         = callbackFunctionIntoBluenet(opcode, &shared_io_buffers);

	// Here the microapp resumes execution, check for incoming interrupts
	handleBluenetInterrupt();

	return result;
}

microapp_sdk_result_t registerInterrupt(interrupt_registration_t* interrupt) {
	for (int i = 0; i < MAX_INTERRUPT_REGISTRATIONS; ++i) {
		if (!interruptRegistrations[i].registered) {
			interruptRegistrations[i].registered = true;
			interruptRegistrations[i].handler    = interrupt->handler;
			interruptRegistrations[i].type       = interrupt->type;
			interruptRegistrations[i].id         = interrupt->id;
			return CS_MICROAPP_SDK_ACK_SUCCESS;
		}
	}
	return CS_MICROAPP_SDK_ACK_ERR_NO_SPACE;
}

microapp_sdk_result_t removeInterruptRegistration(MicroappSdkType type, uint8_t id) {
	for (int i = 0; i < MAX_INTERRUPT_REGISTRATIONS; ++i) {
		if (!interruptRegistrations[i].registered) {
			continue;
		}
		if (interruptRegistrations[i].type == type && interruptRegistrations[i].id == id) {
			interruptRegistrations[i].registered = false;
			return CS_MICROAPP_SDK_ACK_SUCCESS;
		}
	}
	return CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
}

microapp_sdk_result_t callInterrupt(MicroappSdkType type, uint8_t id, microapp_sdk_header_t* interruptHeader) {
	for (int i = 0; i < MAX_INTERRUPT_REGISTRATIONS; ++i) {
		if (!interruptRegistrations[i].registered) {
			continue;
		}
		if (interruptRegistrations[i].type != type) {
			continue;
		}
		if (interruptRegistrations[i].id == id) {
			if (interruptRegistrations[i].handler) {
				return interruptRegistrations[i].handler(interruptHeader);
			}
			// Handler does not exist
			return CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
		}
	}
	// No soft interrupt of this type with this id registered
	return CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND;
}

microapp_sdk_result_t handleInterrupt(microapp_sdk_header_t* interruptHeader) {
	// For all possible interrupt types, get the id from the incoming message
	uint8_t id;
	switch (interruptHeader->messageType) {
		case CS_MICROAPP_SDK_TYPE_PIN: {
			microapp_sdk_pin_t* pinInterrupt = reinterpret_cast<microapp_sdk_pin_t*>(interruptHeader);
			id                            = pinInterrupt->pin;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_BLE: {
			microapp_sdk_ble_t* bleInterrupt = reinterpret_cast<microapp_sdk_ble_t*>(interruptHeader);
			id                            = bleInterrupt->type;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_MESH: {
			microapp_sdk_mesh_t* meshInterrupt = reinterpret_cast<microapp_sdk_mesh_t*>(interruptHeader);
			id                              = meshInterrupt->type;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_MESSAGE: {
			auto messageInterrupt = reinterpret_cast<microapp_sdk_message_t*>(interruptHeader);
			id                    = messageInterrupt->type;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_BLUENET_EVENT: {
			auto eventInterrupt = reinterpret_cast<microapp_sdk_bluenet_event_t*>(interruptHeader);
			id                  = eventInterrupt->type;
			break;
		}
		case CS_MICROAPP_SDK_TYPE_ASSETS: {
			auto assetInterrupt = reinterpret_cast<microapp_sdk_asset_t*>(interruptHeader);
			id                  = assetInterrupt->type;
			break;
		}
		default: {
			return CS_MICROAPP_SDK_ACK_ERR_UNDEFINED;
		}
	}
	// Call the interrupt
	MicroappSdkType type = (MicroappSdkType)interruptHeader->messageType;
	return callInterrupt(type, id, interruptHeader);
}
