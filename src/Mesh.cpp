#include <Mesh.h>

<<<<<<< HEAD
static const uint8_t CMD_SIZE = 1;
uint8_t meshHeaderSize        = sizeof(microapp_mesh_header_t);

void Mesh::sendMeshMsg(uint8_t* msg, uint8_t msg_size, uint8_t stoneId) {

	uint8_t meshHeaderSendSize = sizeof(microapp_mesh_send_header_t);

	global_msg.payload[0] = CS_MICROAPP_COMMAND_MESH;

	microapp_mesh_header_t* send_mesh_cmd_opcode = (microapp_mesh_header_t*)&global_msg.payload[CMD_SIZE];
	send_mesh_cmd_opcode->opcode                 = CS_MICROAPP_COMMAND_MESH_SEND;

	microapp_mesh_send_header_t* send_mesh_cmd_header =
			(microapp_mesh_send_header_t*)&global_msg.payload[CMD_SIZE + meshHeaderSize];
	send_mesh_cmd_header->stoneId = stoneId;

	memcpy(&global_msg.payload[CMD_SIZE + meshHeaderSize + meshHeaderSendSize], msg, msg_size);

	global_msg.length = CMD_SIZE + meshHeaderSize + meshHeaderSendSize + msg_size;
	sendMessage(&global_msg);
}

uint8_t Mesh ::readMeshMsg(uint8_t** msg_ptr, uint8_t* stone_id_ptr) {

	uint8_t meshHeaderReadSize = sizeof(microapp_mesh_read_t);

	global_msg.payload[0]                        = CS_MICROAPP_COMMAND_MESH;
	microapp_mesh_header_t* read_mesh_cmd_opcode = (microapp_mesh_header_t*)&global_msg.payload[CMD_SIZE];
	read_mesh_cmd_opcode->opcode                 = CS_MICROAPP_COMMAND_MESH_READ;

	microapp_mesh_read_t* read_mesh_cmd_header =
			(microapp_mesh_read_t*)&global_msg.payload[CMD_SIZE + meshHeaderSize];
	read_mesh_cmd_header->messageSize = 0;

	global_msg.length = CMD_SIZE + meshHeaderSize + meshHeaderReadSize;
	sendMessage(&global_msg);

	*stone_id_ptr = read_mesh_cmd_header->stoneId;
	*msg_ptr      = read_mesh_cmd_header->message;

	return read_mesh_cmd_header->messageSize;
}

bool Mesh ::available() {

	uint8_t isAvailableHeaderSize = sizeof(microapp_mesh_read_available_t);

	global_msg.payload[0] = CS_MICROAPP_COMMAND_MESH;

	microapp_mesh_header_t* is_available_cmd_opcode = (microapp_mesh_header_t*)&global_msg.payload[CMD_SIZE];
	is_available_cmd_opcode->opcode                 = CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE;

	microapp_mesh_read_available_t* is_available_cmd_header =
			(microapp_mesh_read_available_t*)&global_msg.payload[CMD_SIZE + meshHeaderSize];
	is_available_cmd_header->available = false;

	global_msg.length = CMD_SIZE + meshHeaderSize + isAvailableHeaderSize;
	sendMessage(&global_msg);

	return is_available_cmd_header->available;
}
=======
void Mesh::sendMeshMsg(uint8_t* msg, uint8_t msgSize, uint8_t stoneId) {
	uint8_t* payload = getOutgoingMessagePayload();
	microapp_mesh_send_cmd_t* cmd = (microapp_mesh_send_cmd_t*)(payload);
	cmd->mesh_header.header.cmd   = CS_MICROAPP_COMMAND_MESH;
	cmd->mesh_header.opcode       = CS_MICROAPP_COMMAND_MESH_SEND;
	cmd->stoneId                  = stoneId;

	int msgSizeSent = msgSize;
	if (msgSize > MICROAPP_MAX_MESH_MESSAGE_SIZE) {
		msgSize = MICROAPP_MAX_MESH_MESSAGE_SIZE;
	}

	cmd->dlen = msgSizeSent;
	memcpy(cmd->data, msg, msgSizeSent);

	sendMessage();
}

uint8_t Mesh::readMeshMsg(uint8_t** msgPtr, uint8_t* stoneIdPtr) {
	uint8_t* payloadOut              = getOutgoingMessagePayload();
	microapp_mesh_read_cmd_t* cmdOut = (microapp_mesh_read_cmd_t*)(payloadOut);
	cmdOut->mesh_header.header.cmd   = CS_MICROAPP_COMMAND_MESH;
	cmdOut->mesh_header.opcode       = CS_MICROAPP_COMMAND_MESH_READ;

	sendMessage();

	// Actually the mesh message is written into the wrong buffer by bluenet...
	microapp_mesh_read_cmd_t* cmdIn = cmdOut;

	// uint8_t* payloadIn              = getIncomingMessagePayload();
	// microapp_mesh_read_cmd_t* cmdIn = (microapp_mesh_read_cmd_t*)(payloadIn);

	*stoneIdPtr = cmdIn->stoneId;
	*msgPtr     = cmdIn->data;

	return cmdIn->dlen;
}

bool Mesh::available() {
	uint8_t* payload = getOutgoingMessagePayload();
	microapp_mesh_read_available_cmd_t* cmd = (microapp_mesh_read_available_cmd_t*)(payload);
	cmd->mesh_header.header.cmd             = CS_MICROAPP_COMMAND_MESH;
	cmd->mesh_header.opcode                 = CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE;

	sendMessage();

	return cmd->available;
}
>>>>>>> master
