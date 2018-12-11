#include <M5Stack.h>
#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include "uuid.h"

enum authentication_flags {
	send_key = 0,
	require_random_number = 1,
	send_encrypted_number = 2,
	auth_failed, auth_success = 3,
	waiting = 4
};

authentication_flags	auth_flag;
mbedtls_aes_context		aes;

static uint8_t			encrypted_num[18] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t			auth_key[18];
static uint8_t			_send_rnd_cmd[2] = {0x02, 0x00};
static uint8_t			none[2] = {0, 0};

static uint8_t			n_dev = 0;
static bool				isWaiting = false;

static void notifyCallback_auth(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	switch (pData[1]) {
		case 0x01:
			if (pData[2] == 0x01) {
				auth_flag = require_random_number;
			}
			else {
				auth_flag = auth_failed;
			}
			break;
		case 0x02:
			if (pData[2] == 0x01) {
				mbedtls_aes_init(&aes);
				mbedtls_aes_setkey_enc(&aes, (auth_key + 2), 128);
				mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, pData + 3, encrypted_num + 2);
				mbedtls_aes_free(&aes);
				auth_flag = send_encrypted_number;
			} else {
				auth_flag = auth_failed;
			}
			break;
		case 0x03:
			if (pData[2] == 0x01) {
				auth_flag = auth_success;
			}
			else if (pData[2] == 0x04) {
				auth_flag = send_key;
			}
			break;
		default:
			auth_flag = auth_failed;
	}
}

static void notifyCallback_heartrate(BLERemoteCharacteristic* pHRMMeasureCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	Serial.printf("Get Heart Rate: ");
	Serial.printf("%d\n", pData[1]);
	isWaiting = false;
}

class DeviceSearcher: public BLEAdvertisedDeviceCallbacks {
public:
	void setDevAddr(std::string addr) {
		target_addr = addr;
	}
	
	void onResult (BLEAdvertisedDevice advertisedDevice) {
		std::string addr_now = advertisedDevice.getAddress().toString();
		if (addr_now.compare(target_addr) == 0) {
			M5.Lcd.printf("Target found: %s\n", addr_now.c_str());
			pServerAddress = new BLEAddress(advertisedDevice.getAddress());
			advertisedDevice.getScan()->stop();
			f_found = true;
		}
	}
	
	bool isFound() {
		return f_found;
	}
	
	BLEAddress * getServAddr() {
		return pServerAddress;
	}
	
private:
	bool			f_found = false;
	std::string		target_addr;
	BLEAddress		* pServerAddress;
};

class MiBand2 {
public:
	MiBand2(std::string addr, const uint8_t * key, const char * devName) {
		dev_addr = addr;
		dev_name = devName;
		memcpy(auth_key, key, 18);
	}
	
	~MiBand2() {}
	
	std::string getName() {
		return dev_name;
	}
	
	bool scan4Device(uint8_t timeout) {
		DeviceSearcher * ds = new DeviceSearcher();
		ds->setDevAddr(dev_addr);
		
		BLEDevice::init("");
		BLEScan* pBLEScan = BLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(ds);
		pBLEScan->setActiveScan(true);
		pBLEScan->start(timeout);
		
		if (!ds->isFound()) {
			return false;
		} else {
			pServerAddress = ds->getServAddr();
			return true;
		}
	}
	
	bool connect2Server(BLEAddress pAddress) {
		BLEClient * pClient = BLEDevice::createClient();
		pClient->connect(pAddress);
		Serial.println("Connected to the device.");
		
		// ====================================================================
		// Get useful s/c/d of MI BAND 2
		// --------------------------------------------------------------------
		BLERemoteService * pRemoteService = pClient->getService(service2_uuid);
		if (pRemoteService == nullptr)
			return false;
		M5.Lcd.println(dev_name.c_str());
		pRemoteCharacteristic = pRemoteService->getCharacteristic(auth_characteristic_uuid);
		M5.Lcd.println(" |- CHAR_AUTH");
		if (pRemoteCharacteristic == nullptr)
			return false;
		
		pRemoteService = pClient->getService(alert_sev_uuid);
		M5.Lcd.println("SVC_ALERT");
		pAlertCharacteristic = pRemoteService->getCharacteristic(alert_cha_uuid);
		M5.Lcd.println(" |- CHAR_ALERT");
		
		pRemoteService = pClient->getService(heart_rate_sev_uuid);
		M5.Lcd.println("SVC_HEART_RATE");
		pHRMControlCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_CONTROL);
		M5.Lcd.println(" |- UUID_CHAR_HRM_CONTROL");
		pHRMMeasureCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_MEASURE);
		M5.Lcd.println(" |- CHAR_HRM_MEASURE");
		cccd_hrm = pHRMMeasureCharacteristic->getDescriptor(CCCD_UUID);
		M5.Lcd.println("   |- CCCD_HRM");
		f_connected = true;
		// ====================================================================

		// ====================================================================
		// Bind notification (AUTH)
		// --------------------------------------------------------------------
		pRemoteCharacteristic->registerForNotify(notifyCallback_auth);
		// ====================================================================
		return true;
	}
	
	void authStart() {
		auth_flag = require_random_number;
		BLERemoteDescriptor* pauth_descripter;
		pauth_descripter = pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
		M5.Lcd.println("   |- CCCD_AUTH");
		pauth_descripter->writeValue(auth_key, 2, true);
		Serial.println("# Sent {0x01, 0x00} to CCCD_AUTH");
		while (auth_flag != auth_success) {
			Serial.printf("# AUTH_FLAG: %d\n", auth_flag);
			authentication_flags seaved_flag = auth_flag;
			auth_flag = waiting;
			switch (seaved_flag) {
				case send_key:
					pRemoteCharacteristic->writeValue(auth_key, 18);
					Serial.println("# Sent KEY to CCCD_AUTH");
					break;
				case require_random_number:
					pRemoteCharacteristic->writeValue(_send_rnd_cmd, 2);
					Serial.println("# Sent RND_CMD to CCCD_AUTH");
					break;
				case send_encrypted_number:
					pRemoteCharacteristic->writeValue(encrypted_num, 18);
					Serial.println("# Sent ENCRYPTED_NUM to CCCD_AUTH");
					break;
				default:
				;
			}
			if (auth_flag == seaved_flag) {
				auth_flag = waiting;
			}
			delay(100);
		}
		pauth_descripter->writeValue(none, 2, true);
		Serial.println("# Sent NULL to CCCD_AUTH. AUTH process finished.");
		while (!f_connected && (auth_flag == auth_success));
		Serial.println("# Auth succeed.");
	}
	
	void request4HRM() {
		if (!isWaiting) {
			Serial.println("# REQ sending...");
			isWaiting = true;
			pHRMMeasureCharacteristic->registerForNotify(notifyCallback_heartrate);
			pHRMControlCharacteristic->writeValue(HRM_ONESHOT_STOP, 3, true);
			pHRMControlCharacteristic->writeValue(HRM_ONESHOT_START, 3, true);
			Serial.println("# REQ sent.");
		}
	}
	
	bool init(uint8_t timeout) {
		if (!scan4Device(timeout)) {
			Serial.println("Device not found");
			return false;
		}
		Serial.println("Connceting to services...");
		connect2Server(*pServerAddress);
		authStart();
		cccd_hrm->writeValue(HRM_NOTIFICATION, 2, true);
		n_dev++;
		return true;
	}

private:
	bool					f_found = false;
	bool					f_connected = false;

	std::string				dev_addr;
	std::string				dev_name;
	BLEAddress				* pServerAddress;
	BLERemoteCharacteristic	* pRemoteCharacteristic;
	BLERemoteCharacteristic	* pAlertCharacteristic;
	BLERemoteCharacteristic	* pHRMMeasureCharacteristic;
	BLERemoteCharacteristic	* pHRMControlCharacteristic;
	BLERemoteDescriptor		* cccd_hrm;
};