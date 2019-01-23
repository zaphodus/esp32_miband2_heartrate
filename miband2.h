#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include "uuid.h"
#include "utilize.h"

enum authentication_flags {
	send_key = 0,
	require_random_number = 1,
	send_encrypted_number = 2,
	auth_failed, auth_success = 3,
	waiting = 4
};

enum dflag {
	error = -1,
	idle = 0,
	scanning = 1,
	connecting2Dev = 2,
	connecting2Serv = 3,
	established = 4,
	waiting4data = 5
};

authentication_flags	auth_flag;
mbedtls_aes_context		aes;
dflag					status = idle;

static uint8_t			encrypted_num[18] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t			auth_key[18];
static uint8_t			_send_rnd_cmd[2] = {0x02, 0x00};
static uint8_t			none[2] = {0, 0};

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
	status = idle;
	led_blink(LED_PIN, 500, 1);
	Serial.printf("Get Heart Rate: ");
	Serial.printf("%d\n", pData[1]);
}

class DeviceSearcher: public BLEAdvertisedDeviceCallbacks {
public:
	void setDevAddr(std::string addr) {
		target_addr = addr;
	}
	
	void onResult (BLEAdvertisedDevice advertisedDevice) {
		std::string addr_now = advertisedDevice.getAddress().toString();
		if (addr_now.compare(target_addr) == 0) {
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
	MiBand2(std::string addr, const uint8_t * key) {
		dev_addr = addr;
		memcpy(auth_key, key, 18);
	}
	
	~MiBand2() {
		pClient->disconnect();
		log2("# Operation finished.");
	}
	
	bool scan4Device(uint8_t timeout) {
		DeviceSearcher * ds = new DeviceSearcher();
		ds->setDevAddr(dev_addr);
		
		BLEScan* pBLEScan = BLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(ds);
		pBLEScan->setActiveScan(true);
		pBLEScan->start(timeout);
		
		if (!ds->isFound()) {
			return false;
		} else {
			pServerAddress = ds->getServAddr();
			pClient = BLEDevice::createClient();
			return true;
		}
	}
	
	bool connect2Server(BLEAddress pAddress) {
		pClient->connect(pAddress);
		led_blink(LED_PIN, 100, 5);
		log2("Connected to the device.");
		
		// ====================================================================
		// Get useful s/c/d of MI BAND 2
		// --------------------------------------------------------------------
		BLERemoteService * pRemoteService = pClient->getService(service2_uuid);
		if (pRemoteService == nullptr)
			return false;
		log2("MIBAND2");
		pRemoteCharacteristic = pRemoteService->getCharacteristic(auth_characteristic_uuid);
		log2(" |- CHAR_AUTH");
		if (pRemoteCharacteristic == nullptr)
			return false;
		
		pRemoteService = pClient->getService(alert_sev_uuid);
		log2("SVC_ALERT");
		pAlertCharacteristic = pRemoteService->getCharacteristic(alert_cha_uuid);
		log2(" |- CHAR_ALERT");
		
		pRemoteService = pClient->getService(heart_rate_sev_uuid);
		log2("SVC_HEART_RATE");
		pHRMControlCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_CONTROL);
		log2(" |- UUID_CHAR_HRM_CONTROL");
		pHRMMeasureCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_MEASURE);
		log2(" |- CHAR_HRM_MEASURE");
		cccd_hrm = pHRMMeasureCharacteristic->getDescriptor(CCCD_UUID);
		log2("   |- CCCD_HRM");
		f_connected = true;
		// ====================================================================

		// ====================================================================
		// Bind notification
		// --------------------------------------------------------------------
		pRemoteCharacteristic->registerForNotify(notifyCallback_auth);
		pHRMMeasureCharacteristic->registerForNotify(notifyCallback_heartrate);
		// ====================================================================
		return true;
	}
	
	void authStart() {
		auth_flag = require_random_number;
		BLERemoteDescriptor* pauth_descripter;
		pauth_descripter = pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
		log2("   |- CCCD_AUTH");
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
		log2("# Auth succeed.");
		cccd_hrm->writeValue(HRM_NOTIFICATION, 2, true);
		log2("# Listening on the HRM_NOTIFICATION.");
	}
	
	void startHRM() {
		Serial.println("# Sending HRM command...");
		pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_STOP, 3, true);
		pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_START, 3, true);
		Serial.println("# Sent.");
	}
	
	void startHRM_oneshot() {
		if (status != waiting4data) {
			Serial.println("# Sending HRM-OS command...");
			pHRMControlCharacteristic->writeValue(HRM_ONESHOT_STOP, 3, true);
			pHRMControlCharacteristic->writeValue(HRM_ONESHOT_START, 3, true);
			Serial.println("# Sent.");
			status = waiting4data;
		} else {
			delay(20);
		}
	}
	
	void init(uint8_t timeout) {
		log2(dev_addr);
		log2("Scanning for device...");
		if (!scan4Device(timeout)) {
			log2("Device not found");
			return;
		}

		led_blink(LED_PIN, 50, 5);
		log2("Device found");
		log2("Connceting to services...");
		if (!connect2Server(*pServerAddress)) {
			log2("! Failed to connect to services");
			return;
		}
		led_blink(22, -1, 0);
		authStart();
		status = established;

	}
	
	void deinit() {
		pClient->disconnect();
		log2("# Operation finished.");
	}
	
private:
	bool					f_found = false;
	bool					f_connected = false;

	std::string				dev_addr;
	BLEClient				* pClient;
	BLEAddress				* pServerAddress;
	BLERemoteCharacteristic	* pRemoteCharacteristic;
	BLERemoteCharacteristic	* pAlertCharacteristic;
	BLERemoteCharacteristic	* pHRMMeasureCharacteristic;
	BLERemoteCharacteristic	* pHRMControlCharacteristic;
	BLERemoteDescriptor		* cccd_hrm;
};