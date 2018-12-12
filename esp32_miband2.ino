#include "miband2.h"

const std::string MI_LAB = "f7:f3:ef:13:b1:3d";

// Once the KEY is changed, MI Band 2 will see your device as a new client
uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};

MiBand2 dev(MI_LAB, _KEY);
bool start = false;

void setup() {
	M5.begin();
	M5.Lcd.fillScreen(BLUE);
	M5.Lcd.setTextColor(WHITE);
	
	Serial.begin(115200);

	M5.Lcd.setCursor(0, 0);
	
	log2("Press A to start");

	while (1) {
		M5.update();
		if (M5.BtnA.wasPressed()) {
			break;
		}
		delay(20);
	}

	BLEDevice::init("M5Stack");
	dev.init(30);
	start = true;
}

void loop() {
	if (start) {
		dev.startHRM_oneshot();
	}
	M5.update();
	if (M5.BtnC.wasPressed()) {
		M5.Lcd.setTextSize(1);
		dev.deinit();
		delay(3000);
		M5.powerOFF();
	}
}
