#include "miband2.h"

const std::string MI_LAB = "f7:f3:ef:13:b1:3d";

// Once the KEY is changed, MI Band 2 will see your device as a new client
// uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};
uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x19, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};

MiBand2 dev(MI_LAB, _KEY);
bool start = false;

void setup() {
	pinMode(LED_PIN, OUTPUT);
	pinMode(22, OUTPUT);
	pinMode(SW_PIN, INPUT);
	
	digitalWrite(LED_PIN, 0);
	digitalWrite(22, 1);
	
	Serial.begin(115200);
	log2("Waiting for start...");
	wait4switch(SW_PIN);
	led_blink(LED_PIN, 20, 1);
	BLEDevice::init("ESP-WROOM-32");
	dev.init(30);
	start = true;
}

void loop() {
	if (start) {
		dev.startHRM_oneshot();
	}
}
