#include <Arduino.h>

#include <tensorflow/lite/version.h>

// #include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "hello_world_model_data.h"

boolean ready = false;

tflite::MicroErrorReporter tflErrorReporter;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

const size_t tensorArenaSize = 40;
uint8_t tensorArena[tensorArenaSize] __attribute__((aligned(16)));

// tflite::AllOpsResolver tflOpsresolver;
tflite::MicroMutableOpResolver<5> tflOpsresolver(&tflErrorReporter);


void setup() {
	Serial.print("\n\n Tensorflow in microapp: ");
	Serial.println(TFLITE_VERSION_STRING);

	tflModel = tflite::GetModel(g_hello_world_model_data);
	if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
		Serial.println("Error: schema mismatch");
		return;
	}

	if (tflOpsresolver.AddRelu() != kTfLiteOk) {
		Serial.println("Error: add Relu op");
		return;
	}
	if (tflOpsresolver.AddDequantize() != kTfLiteOk) {
		Serial.println("Error: add Relu op");
		return;
	}
	if (tflOpsresolver.AddQuantize() != kTfLiteOk) {
		Serial.println("Error: add Relu op");
		return;
	}
	if (tflOpsresolver.AddFullyConnected() != kTfLiteOk) {
		Serial.println("Error: add Relu op");
		return;
	}
	if (tflOpsresolver.AddReshape() != kTfLiteOk) {
		Serial.println("Error: add Relu op");
		return;
	}

	static tflite::MicroInterpreter static_interpreter(tflModel, tflOpsresolver, tensorArena, tensorArenaSize, &tflErrorReporter, nullptr);
	tflInterpreter = &static_interpreter;
	tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsresolver, tensorArena, tensorArenaSize, &tflErrorReporter);
	tflInterpreter->AllocateTensors();

	tflInputTensor = tflInterpreter->input(0);
	tflOutputTensor = tflInterpreter->output(0);

	int bytesUsed = tflInterpreter->arena_used_bytes();
	Serial.print("Bytes used: ");
	Serial.println(bytesUsed);

	ready = true;
}


void loop() {
	if (!ready) {
		Serial.print("Setup was not successfull. ");
		Serial.println("Aborting loop");
		return;
	}

	TfLiteStatus invokeStatus = tflInterpreter->Invoke();
	if (invokeStatus != kTfLiteOk) {
		Serial.println("Invoke failed");
		return;
	}

	Serial.println(g_hello_world_model_data_size);
}
