#include <Mesh.h>
#include <Serial.h>

int softInterruptMesh(void* args, void* buf) {
	microapp_mesh_read_cmd_t* msg = (microapp_mesh_read_cmd_t*)buf;
	return Mesh.handleIncomingMeshMsg(msg);
}

MeshClass::MeshClass() : _registeredIncomingMeshMsgHandler(nullptr), _stoneId(0) {}

bool MeshClass::listen() {
	// Register soft interrupt locally
	interrupt_registration_t softInterrupt;
	// Since we only have one type of mesh interrupt, id is always 0
	softInterrupt.id = 0;
	softInterrupt.type = SOFT_INTERRUPT_TYPE_MESH;
	softInterrupt.softInterruptFunc = softInterruptMesh;
	int result = registerSoftInterrupt(&softInterrupt);
	if (result != ERR_MICROAPP_SUCCESS) {
		// No empty interrupt slots available
		return false;
	}

	// Also send a command to bluenet that we want to listen to mesh
	uint8_t* payload = getOutgoingMessagePayload();
	microapp_mesh_cmd_t* mesh_cmd = (microapp_mesh_cmd_t*)(payload);
	mesh_cmd->header.ack = false;
	mesh_cmd->header.cmd = CS_MICROAPP_COMMAND_MESH;
	mesh_cmd->header.id = softInterrupt.id;
	mesh_cmd->opcode = CS_MICROAPP_COMMAND_MESH_READ_SET_HANDLER;

	sendMessage();

	return mesh_cmd->header.ack;
}

int MeshClass::handleIncomingMeshMsg(microapp_mesh_read_cmd_t* msg) {
	// If a handler is registered, we do not need to copy anything to the buffer,
	// since the handler will deal with it right away.
	// The microapp's softInterrupt handler has copied the msg to a localCopy
	// so there is no worry of overwriting the msg upon a bluenet roundtrip
	if (_registeredIncomingMeshMsgHandler != nullptr) {
		MeshMsg handlerMsg = MeshMsg(msg->stoneId, msg->data, msg->dlen);
		_registeredIncomingMeshMsgHandler(handlerMsg);
		return ERR_MICROAPP_SUCCESS;
	}
	// Add msg to buffer or discard if full
	// Q: is it not more logical to discard oldest?
	// that will lead to more memcpy calls however
	bool full = true;
	int i;
	for (i=0; i<MESH_MSG_BUFFER_LEN; i++) {
		if (!_incomingMeshMsgBuffer[i].filled) {
			full = false;
			break;
		}
	}
	if (full) {
		// discard message
		return ERR_MICROAPP_NO_SPACE;
	}
	MeshMsgBufferEntry& copy = _incomingMeshMsgBuffer[i];
	copy.stoneId = msg->stoneId;
	copy.dlen = msg->dlen;
	memcpy(copy.data, msg->data, msg->dlen);
	copy.filled = true;

	return 0;
}

void MeshClass::setIncomingMeshMsgHandler(void (*handler)(MeshMsg)) {
	_registeredIncomingMeshMsgHandler = handler;
}

bool MeshClass::available() {
	for (int i=0; i<MESH_MSG_BUFFER_LEN; i++) {
		if (_incomingMeshMsgBuffer[i].filled) {
			return true;
		}
	}
	return false;
}

void MeshClass::readMeshMsg(MeshMsg* msg) {
	for (int i=MESH_MSG_BUFFER_LEN-1; i>=0; i--) {
		if (_incomingMeshMsgBuffer[i].filled) {
			// copy message data to another location where it can't be overwritten by incoming messages
			// (do not check if _availableMeshMsg was already filled, just overwrite)
			_availableMeshMsg.filled = true;
			memcpy(_availableMeshMsg.data, _incomingMeshMsgBuffer[i].data, MICROAPP_MAX_MESH_MESSAGE_SIZE);
			// create a mesh message to return to the user
			*msg = MeshMsg(	_incomingMeshMsgBuffer[i].stoneId,
							_availableMeshMsg.data,
							_incomingMeshMsgBuffer[i].dlen);

			// free the incoming buffer entry
			_incomingMeshMsgBuffer[i].filled = false;
			return;
		}
	}
}

void MeshClass::sendMeshMsg(uint8_t* msg, uint8_t msgSize, uint8_t stoneId) {
	uint8_t* payload = getOutgoingMessagePayload();
	microapp_mesh_send_cmd_t* cmd = (microapp_mesh_send_cmd_t*)(payload);
	cmd->meshHeader.header.cmd    = CS_MICROAPP_COMMAND_MESH;
	cmd->meshHeader.opcode        = CS_MICROAPP_COMMAND_MESH_SEND;
	cmd->stoneId                  = stoneId;

	int msgSizeSent = msgSize;
	if (msgSize > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
		msgSize = MICROAPP_MAX_MESH_MESSAGE_SIZE;
	}

	cmd->dlen = msgSizeSent;
	memcpy(cmd->data, msg, msgSizeSent);

	sendMessage();
}

short MeshClass::id() {
	// First check if we already cached the id before
	if (_stoneId != 0) {
		return _stoneId;
	}
	// If not, ask bluenet via a MESH_GET_INFO message
	uint8_t* payload = getOutgoingMessagePayload();
	microapp_mesh_info_cmd_t* cmd = (microapp_mesh_info_cmd_t*)(payload);
	cmd->meshHeader.header.cmd = CS_MICROAPP_COMMAND_MESH;
	cmd->meshHeader.header.ack = false;
	cmd->meshHeader.opcode = CS_MICROAPP_COMMAND_MESH_GET_INFO;
	sendMessage();

	if (cmd->meshHeader.header.ack) {
		_stoneId = cmd->stoneId;
	}
	return _stoneId;
}