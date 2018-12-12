#include <M5Stack.h>

void log2(std::string s) {
	M5.lcd.println(s.c_str());
	Serial.println(s.c_str());
}