#include <Arduino.h>
#include <Mesh.h>

const uint8_t APP_IDENTIFIER = 0xFE;
const uint16_t PINGS_PER_SESSION = 100;
const uint16_t ELECTION_TICKS = 100; // loop ticks
const uint16_t TIMEOUT_TICKS = 200; // loop ticks

// determines where to send session results to
const uint8_t CROWNSTONE_HUB_ID = 27; // 27 for almende sphere, 13 for zuidwal sphere

// message types for inter-crownstone messaging
enum StatisticsMicroappMessageType {
	SESSION_START = 0,
	SESSION_PING = 1,
	SESSION_END_ELECTION_START = 2,
	SESSION_RESULT = 3,
	ELECTION_APPLICATION = 4,
	ELECTION_UPDATE = 5,
	ELECTION_RESULT = 6
};

// state that the app can be in
enum StatisticsMicroappState {
	WAITING_FOR_SESSION_START = 0,
	SESSION_COUNTER = 1,
	SESSION_LEADER = 2,
	ELECTION_APPLICANT = 3,
	ELECTION_LEADER = 4,
	WAITING_FOR_LEADER_ACCEPTANCE = 5
};

uint8_t ownId = 0;
uint16_t sessionsSinceLastLeadership = 1;
StatisticsMicroappState state;
StatisticsMicroappState newState;
uint8_t sessionPingsReceived = 0;
uint8_t sessionPingsSent = 0;
uint8_t sessionLeader = 0;
bool completedSessionFlag = false;
bool sendSessionResultFlag = false;
bool sendElectionUpdateFlag = false;
bool sendApplicationFlag = false;
uint16_t electionCounter = 0;
uint8_t currentBestCandidate = 0;
uint16_t currentBestCandidateScore = 0;
uint8_t meshMsgPayload[MAX_MICROAPP_MESH_PAYLOAD_SIZE];
uint16_t timeoutCounter = 0;

void onMeshMessage(MeshMsg msg) {
	if (msg.dataPtr[0] != APP_IDENTIFIER) {
		return;
	}
	switch (msg.dataPtr[1]) {
		case SESSION_START: {
			timeoutCounter = 0;
			// listen to session starts in any state
			sessionLeader = msg.stoneId;
			sessionPingsReceived = 0;
			sessionPingsSent = 0;
			electionCounter = 0;
			// clear candidate scores
			currentBestCandidate = 0;
			currentBestCandidateScore = 0;
			// clear flags
			completedSessionFlag = false;
			sendSessionResultFlag = false;
			sendElectionUpdateFlag = false;
			sendApplicationFlag = false;
			// new state will be session counter
			newState = SESSION_COUNTER;
			break;
		}
		case SESSION_PING: {
			timeoutCounter = 0;
			// only increment counter if we are in the session counter state and ping is from known session leader
			if (state == SESSION_COUNTER && msg.stoneId == sessionLeader) {
				uint8_t pingIndex = msg.dataPtr[2];
				if (pingIndex > sessionPingsSent) {
					sessionPingsReceived++;
					sessionPingsSent = pingIndex;
				}
			}
			break;
		}
		case SESSION_END_ELECTION_START: {
			timeoutCounter = 0;
			// only set completedSessionFlag if we are in the session counter state and completed the session
			if (state == SESSION_COUNTER && msg.stoneId == sessionLeader) {
				sessionPingsSent = msg.dataPtr[2];
				completedSessionFlag = true;
				sendSessionResultFlag = true;
			}
			sendApplicationFlag = true;
			// regardless of state, new state will be election applicant
			newState = ELECTION_APPLICANT;
			break;
		}
		case ELECTION_APPLICATION: {
			// only if we are the electing leader do we listen to applications
			if (state == ELECTION_LEADER) {
				if (msg.dataPtr[2] > currentBestCandidateScore) {
					currentBestCandidate = msg.stoneId;
					currentBestCandidateScore = msg.dataPtr[2];
				}
				// as long as we are receiving applications, broadcast election update in response
				sendElectionUpdateFlag = true;
			}
			break;
		}
		case ELECTION_UPDATE: {
			// check current best score that the leader is advertising. If we don't have a higher score, it's not worth applying any longer this round
			uint8_t currentBestScore = msg.dataPtr[3];
			if (currentBestScore >= sessionsSinceLastLeadership) {
				sendApplicationFlag = false;
			}
			break;
		}
		case ELECTION_RESULT: {
			timeoutCounter = 0;
			// listen to election results in any state. not secure to outside tampering of any kind...
			uint8_t newSessionLeader = msg.dataPtr[2];
			if (newSessionLeader == ownId) {
				sessionsSinceLastLeadership = 0;
				sessionLeader = newSessionLeader;
				newState = SESSION_LEADER;
				return;
			}
			if (newSessionLeader != sessionLeader) {
				sessionLeader = newSessionLeader;
				newState = WAITING_FOR_SESSION_START;
			}
			if (completedSessionFlag) {
				sessionsSinceLastLeadership++;
				// clear flag here to prevent multiple counter increments if election result arrives more than once
				completedSessionFlag = false;
			}
			break;
		}
		default:
			break;
	}
}

void sendSessionResult() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = SESSION_RESULT;
	meshMsgPayload[2] = sessionPingsReceived;
	meshMsgPayload[3] = sessionPingsSent;
	meshMsgPayload[4] = sessionLeader;
	meshMsgPayload[5] = sessionsSinceLastLeadership;
	Mesh.sendMeshMsg(meshMsgPayload, 6, CROWNSTONE_HUB_ID);
}

void sendLeadershipApplication() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = ELECTION_APPLICATION;
	meshMsgPayload[2] = sessionsSinceLastLeadership;
	Mesh.sendMeshMsg(meshMsgPayload, 3);
	// Mesh.sendMeshMsg(meshMsgPayload, 3, sessionLeader);
}

void sendSessionStart() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = SESSION_START;
	meshMsgPayload[2] = timeoutCounter;
	meshMsgPayload[3] = currentBestCandidate;
	Mesh.sendMeshMsg(meshMsgPayload, 4);
}

void sendPing() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = SESSION_PING;
	meshMsgPayload[2] = sessionPingsSent;
	Mesh.sendMeshMsg(meshMsgPayload, 3, 0, true);
}

void sendSessionEndElectionStart() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = SESSION_END_ELECTION_START;
	meshMsgPayload[2] = sessionPingsSent;
	Mesh.sendMeshMsg(meshMsgPayload, 3);
}

void sendElectionUpdate() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = ELECTION_UPDATE;
	meshMsgPayload[2] = currentBestCandidate;
	meshMsgPayload[3] = currentBestCandidateScore;
	Mesh.sendMeshMsg(meshMsgPayload, 4);
}

void sendElectionResult() {
	meshMsgPayload[0] = APP_IDENTIFIER;
	meshMsgPayload[1] = ELECTION_RESULT;
	meshMsgPayload[2] = currentBestCandidate;
	meshMsgPayload[3] = currentBestCandidateScore;
	Mesh.sendMeshMsg(meshMsgPayload, 4);
}

void leaderSession() {
	sessionPingsReceived = 0;
	sessionPingsSent = 0;
	electionCounter = 0;
	timeoutCounter = 0;
	delay(1000); // give others a chance to process election result
	sendSessionStart();
	delay(1000); // give others a chance to process session start
	while (sessionPingsSent++ < PINGS_PER_SESSION) {
		sendPing();
		delay(100);
	}
	delay(900); // give others a chance to process session start
	sendSessionEndElectionStart();
	sessionsSinceLastLeadership = 1;
}

void setup() {
	Serial.begin();
	Serial.print("Mesh statistics app with app ID: ");
	Serial.println(APP_IDENTIFIER);

	newState = WAITING_FOR_SESSION_START;
	state = newState;

	ownId = Mesh.id();

	Mesh.listen();
	Mesh.setIncomingMeshMsgHandler(onMeshMessage);

}

void loop() {
	// if we don't get a directive mesh message for this application, let's force ourselves to be session leader
	if (timeoutCounter++ > TIMEOUT_TICKS) {
		newState = SESSION_LEADER;
	}
	state = newState;
	switch (state) {
		case SESSION_LEADER:
			leaderSession();
			newState = ELECTION_LEADER;
			break;
		case ELECTION_APPLICANT:
			if (sendApplicationFlag) {
				sendLeadershipApplication();
			}
			if (sendSessionResultFlag) {
				sendSessionResult();
				sendSessionResultFlag = false;
			}
			break;
		case ELECTION_LEADER:
			if (sendElectionUpdateFlag) {
				sendElectionUpdate();
				sendElectionUpdateFlag = false;
			}
			if (electionCounter++ < ELECTION_TICKS) {
				return;
			}
			if (currentBestCandidate == 0) {
				// just go again
				newState = SESSION_LEADER;
			}
			else {
				newState = WAITING_FOR_LEADER_ACCEPTANCE;
			}
			break;
		case WAITING_FOR_LEADER_ACCEPTANCE:
			sendElectionResult();
		default:
			break;
	}
}