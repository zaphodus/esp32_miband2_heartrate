#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include "uuid.h"

const std::string MI_ADDR = "f7:f3:ef:13:b1:3d";

uint8_t _KEY [18] =  {0x01, 0x00, 0x28, 0x6b, 0xc5, 0x9d, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};
uint8_t _send_rnd_cmd[2] = {0x02, 0x00};
uint8_t encrypted_num[18] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t none[2] = {0, 0};

static BLEAddress *pServerAddress;
static bool doConnect = false;
static bool connected = false;
static mbedtls_aes_context aes;

static int heartrate_count = 0;

BLERemoteCharacteristic* pRemoteCharacteristic;
BLERemoteCharacteristic* pAlertCharacteristic;
BLERemoteCharacteristic* pAlertNotifyChar;
BLERemoteCharacteristic* pBatteryCharacteristic;
BLERemoteCharacteristic* pHRMMeasureCharacteristic;
BLERemoteCharacteristic* pHRMControlCharacteristic;
BLERemoteDescriptor* cccd_hrm;

enum authentication_flags {
	send_key = 0,
	require_random_number = 1,
	send_encrypted_number = 2,
	auth_failed, auth_success = 3,
	waiting = 4
};

authentication_flags auth_flag = require_random_number;

static void notifyCallback_0(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	Serial.println("Get reply in call back");
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
				mbedtls_aes_setkey_enc( &aes, (_KEY + 2), 128 );
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

static void notifyCallback_1(BLERemoteCharacteristic* pHRMMeasureCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	Serial.printf("Get Heart Rate: ");
	Serial.printf("%d\n", pData[1]);
	heartrate_count++;
	if (heartrate_count == 3) {
		heartrate_count = 0;
		pHRMControlCharacteristic->writeValue(HRM_HEARTBEAT, 1, true);
	}
}

bool connectToServer(BLEAddress pAddress) {
	BLEClient*  pClient  = BLEDevice::createClient();
	pClient->connect(pAddress);
	Serial.println("Connected to the device.");
	
	// ====================================================================
	// Get useful s/c/d of MI BAND 2
	// --------------------------------------------------------------------
	BLERemoteService* pRemoteService = pClient->getService(service2_uuid);
	if (pRemoteService == nullptr)
		return false;
	Serial.println("MIBAND2");
	pRemoteCharacteristic = pRemoteService->getCharacteristic(auth_characteristic_uuid);
	Serial.println(" |- CHAR_AUTH");
	if (pRemoteCharacteristic == nullptr)
		return false;
	
	pRemoteService = pClient->getService(alert_sev_uuid);
	Serial.println("SVC_ALERT");
	pAlertCharacteristic = pRemoteService->getCharacteristic(alert_cha_uuid);
	Serial.println(" |- CHAR_ALERT");
	
	pRemoteService = pClient->getService(heart_rate_sev_uuid);
	Serial.println("SVC_HEART_RATE");
	pHRMControlCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_CONTROL);
	Serial.println(" |- UUID_CHAR_HRM_CONTROL");
	pHRMMeasureCharacteristic = pRemoteService->getCharacteristic(UUID_CHAR_HRM_MEASURE);
	Serial.println(" |- CHAR_HRM_MEASURE");
	cccd_hrm = pHRMMeasureCharacteristic->getDescriptor(CCCD_UUID);
	Serial.println("   |- CCCD_UUID");

	pRemoteCharacteristic->registerForNotify(notifyCallback_0);
	pHRMMeasureCharacteristic->registerForNotify(notifyCallback_1);
	return true;
}

void sendCmd() {
	cccd_hrm->writeValue(HRM_NOTIFICATION, 2, true);
	pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_STOP, 3, true);
	pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_START, 3, true);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
		std::string addr = advertisedDevice.getAddress().toString();
		if (addr.compare(MI_ADDR) == 0) {
			Serial.printf("Target found: %s \n", addr.c_str());
			pServerAddress = new BLEAddress(advertisedDevice.getAddress());
			advertisedDevice.getScan()->stop();
			doConnect = true;
		}
	}
};

void setup() {
	Serial.begin(115200);
	BLEDevice::init("");
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);
	while (!connected) {
		if (doConnect == true) {
			if (connectToServer(*pServerAddress)) {
				Serial.println("We are now connected to the BLE Server.");
				connected = true;
			}
		}
	}
	
	BLERemoteDescriptor* pauth_descripter;
	pauth_descripter = pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
	Serial.println("NOTIFICATION_DESCRIPTOR");
	pauth_descripter->writeValue(_KEY, 2, true);
	Serial.println("pauth_descripter->writeValue(_KEY, 2, true)");
	while (auth_flag != auth_success) {
		Serial.println(auth_flag);
		authentication_flags seaved_flag = auth_flag;
		auth_flag = waiting;
		switch (seaved_flag) {
			case send_key:
				pRemoteCharacteristic->writeValue(_KEY, 18);
				break;
			case require_random_number:
				pRemoteCharacteristic->writeValue(_send_rnd_cmd, 2);
				break;
			case send_encrypted_number:
				pRemoteCharacteristic->writeValue(encrypted_num, 18);
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
	Serial.println("pauth_descripter->writeValue(none, 2, true)");
	sendCmd();
}

void loop() {
	if (connected && (auth_flag == auth_success)) {
		Serial.println("Auth success.");
	}
	delay(5000);
}
