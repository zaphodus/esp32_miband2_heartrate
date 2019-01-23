#include <M5Stack.h>

void log2(std::string s) {
	M5.lcd.println(s.c_str());
	Serial.println(s.c_str());
}

void writeOut(const char * path, const char * message, const char * method)
{
	Serial.printf("Appending to file: %s\n", path);
	
	File file = SD.open(path, method);
	if (!file) {
		Serial.println("Failed to open file for appending");
		return;
	}
	if (file.print(message)) {
		Serial.println("Message appended");
	} else {
		Serial.println("Append failed");
	}
	file.close();
}

bool mountSD()
{
	if (!SD.begin()) {
		Serial.println("SD Mount Failed");
		return false;
	} else {
		Serial.println("SD Mounted");
		return true;
	}
}

void fileNameGen(char * container, const char * folder, const char * front)
{
	int fileNum = 0;

	if (!SD.exists(folder)) {
		SD.mkdir(folder);
	}
	while (1) {
		sprintf(container, "%s/%s%03d.csv", folder, front, fileNum);   
		if (!SD.exists(container)) {
			break;
		}else {
			fileNum++;
		}
	}
	M5.lcd.println(container);
}

void wait4ButtonA()
{
	while (1) {
		M5.update();
		if (M5.BtnA.wasPressed()) {
			break;
		}
		delay(20);
	}
}

void wait4ButtonB()
{
	while (1) {
		M5.update();
		if (M5.BtnB.wasPressed()) {
			break;
		}
		delay(20);
	}
}

void wait4ButtonC()
{
	while (1) {
		M5.update();
		if (M5.BtnC.wasPressed()) {
			break;
		}
		delay(20);
	}
}