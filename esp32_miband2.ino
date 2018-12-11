#include "miband2.h"

#define N 2

const std::string MI_LAB = "f7:f3:ef:13:b1:3d";
const std::string MI_MY = "e5:f1:8f:44:db:29";

// Once the KEY is changed, MI Band 2 will see your device as a new client
uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};

MiBand2 dev_all[N] = {
	MiBand2(MI_LAB, _KEY, "labDev"),
	MiBand2(MI_MY, _KEY, "myDev")
};

void dispatcher() {
	for (int i=0; i<N; i++) {
		dev_all[i].init(30);
		delay(2000);
	}
	while (1) {
		for (int i=0; i<N; i++) {
			Serial.println(dev_all[i].getName().c_str());
			dev_all[i].request4HRM();
			delay(10);
		}
	}
}

void setup() {
	M5.begin();
	M5.Lcd.fillScreen(BLUE);
	M5.Lcd.setTextColor(WHITE);

	Serial.begin(115200);

	M5.Lcd.setCursor(0, 0);
	M5.Lcd.println("Scanning...");
	
	dispatcher();
}

void loop() {
	delay(12000);
}
