#include <M5Stack.h>
#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include "uuid.h"

class DeviceSearcher: public BLEAdvertisedDeviceCallbacks {
	std::string addr;
	
	DeviceSearcher (std::string addr) {
		this.addr = addr;
	}
	
	void onResult (BLEAdvertisedDevice advertisedDevice) {
		std::string addr_now = advertisedDevice.getAddress().toString();
		if (addr_now.compare(this.addr) == 0) {
			M5.Lcd.printf("Target found: %s\n", addr_now.c_str());
			pServerAddress = new BLEAddress(advertisedDevice.getAddress());
			advertisedDevice.getScan()->stop();
			doConnect = true;
		}
	}
};

class MiBand2 {
public:
	MiBand2(std::string addr);
	~MiBand2();
	bool scan4Device(uint8_t timeout);
	
	
private:
	std::string				addr;
	BLERemoteCharacteristic	* pRemoteCharacteristic;
	BLERemoteCharacteristic	* pAlertCharacteristic;
	BLERemoteCharacteristic	* pHRMMeasureCharacteristic;
	BLERemoteCharacteristic	* pHRMControlCharacteristic;
	BLERemoteDescriptor		* cccd_hrm;
	
	
}