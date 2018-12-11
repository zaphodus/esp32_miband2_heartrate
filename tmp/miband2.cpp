#include "miband2.h"

MiBand2::MiBand2 (std::string addr) {
	this.addr = addr;
}

MiBand2::~MiBand2 () {}

MiBand2::scan4Device (uint8_t timeout) {
	BLEDevice::init("");
	BLEScan * pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(timeout);
}