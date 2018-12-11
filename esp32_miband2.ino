#include "miband2.h"

const std::string MI_LAB = "f7:f3:ef:13:b1:3d";
const std::string MI_MY = "e5:f1:8f:44:db:29";

// Once the KEY is changed, MI Band 2 will see your device as a new client
uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};

void setup() {
	M5.begin();
	M5.Lcd.fillScreen(BLUE);
	M5.Lcd.setTextColor(WHITE);

	Serial.begin(115200);

	M5.Lcd.setCursor(0, 0);
	M5.Lcd.println("Scanning...");
	
	MiBand2 dev1(MI_MY, _KEY, "myDev");
	MiBand2 dev2(MI_LAB, _KEY, "labDev");
	dev1.run(30);
	delay(3000);
	dev2.run(30);
}

void loop() {
	delay(12000);
}
