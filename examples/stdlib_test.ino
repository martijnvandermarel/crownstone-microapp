//
// Test to see size of program with std lib
//

#include <Arduino.h>
#include <cstring>

const char my_string[] = "Hello world\n";

void setup() {
	Serial.println("Setup");
}

void loop() {
	// strlen
	Serial.println(strlen(my_string));

	// memcpy
	char my_second_string[MAX_STRING_SIZE];
	memcpy(my_second_string, my_string, strlen(my_string));

	// memcmp
	int res = memcmp(my_string, my_second_string, strlen(my_string));
	Serial.println(res);
}
