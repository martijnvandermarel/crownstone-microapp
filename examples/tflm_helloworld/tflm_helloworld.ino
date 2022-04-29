#include <Arduino.h>

#include <tensorflow/lite/version.h>
#include <tensorflow/lite/micro/micro_interpreter.h>

#include "hello_world_model_data.h"

const tflite::Model* tflModel = nullptr;

void setup() {
	Serial.println(TFLITE_VERSION_STRING);

	tflModel = tflite::GetModel(g_hello_world_model_data);
}


void loop() {
	Serial.println(g_hello_world_model_data_size);
}
