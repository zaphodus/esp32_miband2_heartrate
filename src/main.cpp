#include <M5Stack.h>
#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include "uuid.h"

const std::string MI_ADDR = "f7:f3:ef:13:b1:3d";

// Once the KEY is changed, MI Band 2 will see your device as a new client
uint8_t _KEY [18] =  {0x01, 0x00, 0x82, 0xb6, 0x5c, 0xd9, 0x91, 0x95, 0x9a, 0x72, 0xe5, 0xcc, 0xb7, 0xaf, 0x62, 0x33, 0xee, 0x35};
uint8_t _send_rnd_cmd[2] = {0x02, 0x00};
uint8_t encrypted_num[18] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t none[2] = {0, 0};

static BLEAddress *pServerAddress;
static bool doConnect = false;
static bool connected = false;
static mbedtls_aes_context aes;

static uint8_t flag_hrm = 0;

//hw_timer_t * timer = NULL;

BLERemoteCharacteristic* pRemoteCharacteristic;
BLERemoteCharacteristic* pAlertCharacteristic;
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

//void IRAM_ATTR onTimer() {
//	pHRMControlCharacteristic->writeValue(HRM_HEARTBEAT, 1, true);
//	Serial.println("# Heart beat packet has been sent");
//}

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

static void notifyCallback_heartrate(BLERemoteCharacteristic* pHRMMeasureCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
	Serial.printf("Get Heart Rate: ");
	Serial.printf("%d\n", pData[1]);
	flag_hrm = 1;
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
	M5.Lcd.println("MIBAND2");
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
	// ====================================================================

	// ====================================================================
	// Bind notification
	// --------------------------------------------------------------------
	pRemoteCharacteristic->registerForNotify(notifyCallback_auth);
	pHRMMeasureCharacteristic->registerForNotify(notifyCallback_heartrate);
	// ====================================================================
	return true;
}

void sendCmd() {
	cccd_hrm->writeValue(HRM_NOTIFICATION, 2, true);
	pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_STOP, 3, true);
	pHRMControlCharacteristic->writeValue(HRM_CONTINUOUS_START, 3, true);
}

// ====================================================================
// Callback of BT scanner
// --------------------------------------------------------------------
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
		std::string addr = advertisedDevice.getAddress().toString();
		if (addr.compare(MI_ADDR) == 0) {
			M5.Lcd.printf("Target found: %s\n", addr.c_str());
			pServerAddress = new BLEAddress(advertisedDevice.getAddress());
			advertisedDevice.getScan()->stop();
			doConnect = true;
		}
	}
};
// ====================================================================

void setup() {
	M5.begin();
	M5.Lcd.fillScreen(BLUE);
	M5.Lcd.setTextColor(WHITE);

	Serial.begin(115200);

	// ====================================================================
	// Scan for the target device
	// --------------------------------------------------------------------
  M5.Lcd.setCursor(0, 0);
	M5.Lcd.println("Scanning...");
	BLEDevice::init("");
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);	// Scan last for 30s. If there is no result, failed
	// ====================================================================

	// ====================================================================
	// Connect to the MI Band servers
	// --------------------------------------------------------------------
	while (!connected) {
		if (doConnect == true) {
			if (connectToServer(*pServerAddress)) {
				Serial.println("We are now connected to the BLE Server.");
				connected = true;
			}
		}
	}
	// ====================================================================

	// ====================================================================
	// Start auth
	// --------------------------------------------------------------------
	BLERemoteDescriptor* pauth_descripter;
	pauth_descripter = pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
	M5.Lcd.println("   |- CCCD_AUTH");
	pauth_descripter->writeValue(_KEY, 2, true);
	Serial.println("# Sent {0x01, 0x00} to CCCD_AUTH");
	while (auth_flag != auth_success) {
		Serial.printf("# AUTH_FLAG: %d\n", auth_flag);
		authentication_flags seaved_flag = auth_flag;
		auth_flag = waiting;
		switch (seaved_flag) {
			case send_key:
				pRemoteCharacteristic->writeValue(_KEY, 18);
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
	if (connected && (auth_flag == auth_success)) {
		Serial.println("# Auth success.");
	}
	// ====================================================================
	sendCmd();
}

void loop() {
  if (flag_hrm) {
    pHRMControlCharacteristic->writeValue(HRM_HEARTBEAT, 1, true);
    Serial.println("# Heart beat packet has been sent");
    delay(12000);
  } else {
    delay(.5);
  }
}
