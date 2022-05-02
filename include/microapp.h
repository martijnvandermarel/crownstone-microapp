#pragma once

#include <cstring>

// Get defaults from bluenet
#include <cs_MicroappStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

// SoftInterrupt functions
typedef void (*softInterruptFunction)(void *, void *);

const uint8_t SOFT_INTERRUPT_TYPE_BLE = 1;
const uint8_t SOFT_INTERRUPT_TYPE_PIN = 2;

// Store softInterrupts in the microapp
struct soft_interrupt_t {
    uint8_t type;
    uint8_t id;
    softInterruptFunction softInterruptFunc;
    void *arg;
    bool registered;
};

#define MAX_SOFT_INTERRUPTS 4

extern soft_interrupt_t softInterrupt[MAX_SOFT_INTERRUPTS];

// Create shortened typedefs (it is obvious we are within the microapp here)

typedef microapp_ble_cmd_t ble_cmd_t;
typedef microapp_twi_cmd_t twi_cmd_t;
typedef microapp_pin_cmd_t pin_cmd_t;
typedef microapp_ble_device_t ble_dev_t;

// Create long-form version for who wants

// typedef message_t microapp_message_t;

#define OUTPUT CS_MICROAPP_COMMAND_PIN_WRITE
#define INPUT CS_MICROAPP_COMMAND_PIN_READ
#define TOGGLE CS_MICROAPP_COMMAND_PIN_TOGGLE
#define INPUT_PULLUP CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP

#define CHANGE CS_MICROAPP_COMMAND_VALUE_CHANGE
#define RISING CS_MICROAPP_COMMAND_VALUE_RISING
#define FALLING CS_MICROAPP_COMMAND_VALUE_FALLING

#define I2C_INIT CS_MICROAPP_COMMAND_TWI_INIT
#define I2C_READ CS_MICROAPP_COMMAND_TWI_READ
#define I2C_WRITE CS_MICROAPP_COMMAND_TWI_WRITE

//#define HIGH                 CS_MICROAPP_COMMAND_SWITCH_ON
//#define LOW                  CS_MICROAPP_COMMAND_SWITCH_OFF

const uint8_t LOW = 0;
const uint8_t HIGH = !LOW;

// 10 GPIO pins, 4 buttons, and 4 leds
#define NUMBER_OF_PINS 18

// set a max string size which is equal to the max payload of microapp_message_t
const size_t MAX_STRING_SIZE = MAX_PAYLOAD;

/*
 * Get outgoing/incoming message buffer (can be used for sendMessage);
 */

uint8_t *getOutgoingMessagePayload();

uint8_t *getIncomingMessagePayload();

/**
 * Send a message to the bluenet code. This is the function that is called - in
 * the end - by all the functions that have to reach the microapp code.
 */
int sendMessage();

/**
 * Register a softInterrupt locally.
 */
int registerSoftInterrupt(soft_interrupt_t *interrupt);

/**
 * Evoke a previously registered softInterrupt.
 */
int evokeSoftInterrupt(uint8_t type, uint8_t id, uint8_t *msg);

/**
 * Handle softInterrupts.
 */
int handleSoftInterrupt(microapp_cmd_t *msg);

#ifdef __cplusplus
}
#endif
