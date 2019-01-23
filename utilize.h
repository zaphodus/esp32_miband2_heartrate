#define LED_PIN 23
#define SW_PIN 15

void log2(std::string s)
{
	Serial.println(s.c_str());
}

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