/**
 * A BLE client example that is rich in capabilities.
 */

#include "BLEDevice.h"
#include "HardwareSerial.h"
#include "mbedtls/aes.h"
#include <uuid.h>
#include <lcd.h>
uint8_t _KEY [18] =  {0x01,0x00,0x28,0x6b,0xc5,0x9d,0x91,0x95,0x9a,0x72,0xe5,0xcc,0xb7,0xaf,0x62,0x33,0xee,0x35};
uint8_t _send_rnd_cmd[2] = {0x02,0x00};
uint8_t encrypted_num[18]={0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t none[2] = {0,0};
uint8_t fucker[13] = {0x05,0x01,0xE8,0x80,0x81,0xE5,0xB8,0x88,0xE5,0xA5,0xBD};

static BLEAddress *pServerAddress;
static bool doConnect = false;
static bool connected = false;
static mbedtls_aes_context aes;

BLERemoteCharacteristic* pRemoteCharacteristic;
BLERemoteCharacteristic* pAlertCharacteristic;
BLERemoteCharacteristic* pAlertNotifyChar;
BLERemoteCharacteristic* pBatteryCharacteristic;

enum authentication_flags
{
	send_key=0,
   	require_random_number=1,
   	send_encrypted_number=2,
   	auth_failed,auth_success=3,
	 waiting=4
};

authentication_flags auth_flag= require_random_number;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify) 
{
	switch(pData[1])
		{
			case 0x01:
				{
					if (pData[2]==0x01)
							auth_flag= require_random_number;
					else
					auth_flag= auth_failed;
				}break;
			case 0x02:
				{
					if(pData[2]==0x01)
					{
						mbedtls_aes_init(&aes);
						mbedtls_aes_setkey_enc( &aes,(_KEY+2), 128 );//因为秘钥前加了前缀，所以加上前缀的偏移量
						mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,pData+3,encrypted_num+2);
						mbedtls_aes_free(&aes);
						auth_flag= send_encrypted_number;
					}
					else
						auth_flag= auth_failed;
				}break;
			case 0x03:
				{
					if(pData[2]==0x01)
						auth_flag= auth_success;
					else if (pData[2]==0x04)
						auth_flag= send_key;//因为密钥错了，所以重新发送秘钥覆盖手环中原来保存的秘钥
				}break;
				default:
					auth_flag= auth_failed;
    		}
}

bool connectToServer(BLEAddress pAddress) {
	digitalWrite(32,0.3);
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->connect(pAddress);
    BLERemoteService* pRemoteService = pClient->getService(service2_uuid);
    BLERemoteService* pRemoteService1 = pClient->getService(service1_uuid);
    BLERemoteService* pAlertService = pClient->getService(alert_sev_uuid);
    BLERemoteService* pAlert2Service = pClient->getService(alert_notify_sev_uuid);
    if (pRemoteService == nullptr)
    	return false;
    pRemoteCharacteristic = pRemoteService->getCharacteristic(auth_characteristic_uuid);
    pAlertCharacteristic = pAlertService->getCharacteristic(alert_cha_uuid);
 pAlertNotifyChar = pAlert2Service->getCharacteristic(alert2_cha_uuid);
	pBatteryCharacteristic = pRemoteService1->getCharacteristic(CHARACTERISTIC_BATTERY);
    if (pRemoteCharacteristic == nullptr) 
      	return false;
    pRemoteCharacteristic->registerForNotify(notifyCallback);
	digitalWrite(32,HIGH);
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
   {
    if (advertisedDevice.getName()==std::string("MI Band 2") ) 
		// if(strcmp(advertisedDevice.getAddress().toString().c_str(),"fb:24:d1:c1:3e:f3" ))
		{
		pServerAddress = new BLEAddress(advertisedDevice.getAddress());
		doConnect = true;
		} // Found our server
  	} // onResult
}; // MyAdvertisedDeviceCallbacks

void text_notify(std::__cxx11::string text)
{
	int length = text.length();
	uint8_t per_text[2+length]={0x05,0x01};
	memcpy(per_text+2,text.c_str(),length);
	for (int i=0;i<(length+2);i++)
		Serial.printf("%04x ",per_text[i]);
	pAlertNotifyChar->writeValue(per_text,length+2,true);
}
void setup() {
	Serial.begin(115200);
	pinMode(32,OUTPUT);
	pinMode(33,OUTPUT);
	pinMode(27,OUTPUT);
	digitalWrite(32,HIGH);//蓝
	digitalWrite(33,HIGH);
	digitalWrite(27,HIGH);//红
	BLEDevice::init("");

	// Retrieve a Scanner and set the callback we want to use to be informed when we
	// have detected a new device.  Specify that we want active scanning and start the
	// scan to run for 30 seconds.
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);
	while(!connected)
		{ 
			// If the flag "doConnect" is true then we have scanned for and found the desired
			// BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
			// connected we set the connected flag to be true.
			if (doConnect == true)
				{
				if (connectToServer(*pServerAddress)) 
				{
				//   Serial.println("We are now connected to the BLE Server.");
				connected = true;
				} 
			}
		}
		BLERemoteDescriptor* pauth_descripter;  														  		     //开启手环认证通知
		pauth_descripter=pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));	
		pauth_descripter->writeValue(_KEY,2,true);
	while (auth_flag != auth_success) {
		//   Serial.println(auth_flag);
		authentication_flags seaved_flag=auth_flag;
		auth_flag=waiting;
		switch(seaved_flag)
			{
			case send_key:
				pRemoteCharacteristic->writeValue(_KEY,18); break;
			case require_random_number:
				pRemoteCharacteristic->writeValue(_send_rnd_cmd,2); break;//获取随机数
			case send_encrypted_number:
				{
				// for (int i=0;i<18;i++)
				// 		Serial.printf("%04x ",encrypted_num[i]);
				pRemoteCharacteristic->writeValue(encrypted_num,18);
				} break;
			default:
				;
			}
			if(auth_flag==seaved_flag)
				auth_flag=waiting;
			delay(100);
		} 
		pauth_descripter->writeValue(none,2,true);

	} // End of setup.


	// This is the Arduino main loop function.
	void loop() {
	if(connected&&(auth_flag == auth_success))
		// if(digitalRead(16))
		// 	{
				digitalWrite(33,0.3);
				text_notify("你好mmp\0");
				digitalWrite(33,HIGH);
			// }
	// std::__cxx11::string battery_info;
	//battery_info = pBatteryCharacteristic->readValue();
	// for (int i=0;i<20;i++)
	// 	Serial.printf("%04x ",battery_info[i]);
	//
	delay(15000); // Delay a second between loops.
} // End of loop