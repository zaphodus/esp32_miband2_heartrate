void log2(std::string s)
{
	Serial.println(s.c_str());
}

void led_blink(uint8_t led_pin, uint8_t interval, uint8_t n_blink)
{
	pinMode(led_pin, OUTPUT);
	for (uint8_t i=0; i<n_blink*2; i++) {
		digitalWrite(led_pin, !digitalRead(led_pin));
		delay(interval);
	}
	
}