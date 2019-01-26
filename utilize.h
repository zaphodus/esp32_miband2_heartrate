void log2(std::string s) {
	Serial.println(s.c_str());
}

/*
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
*/
void led_blink(uint8_t led_pin, uint8_t interval, uint8_t n_blink)
{
	digitalWrite(led_pin, 0);
	if (interval == -1) {
		digitalWrite(led_pin, !digitalRead(led_pin));
	} else {
		for (uint8_t i=0; i<n_blink*2; i++) {
			digitalWrite(led_pin, !digitalRead(led_pin));
			delay(interval);
		}
	}
}

void wait4switch(uint8_t sw_pin)
{
	while (1) {
		if (!digitalRead(sw_pin)) {
			break;
		} else {
			delay(1000);
		}
	}
}