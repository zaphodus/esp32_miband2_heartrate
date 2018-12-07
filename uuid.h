// *** SERVER ***
// AUTH_SERVER
static BLEUUID service2_uuid("0000fee1-0000-1000-8000-00805f9b34fb");
// HRM_SERV
static BLEUUID heart_rate_sev_uuid("0000180d-0000-1000-8000-00805f9b34fb");
// Other services (not used)
static BLEUUID service1_uuid("0000fee0-0000-1000-8000-00805f9b34fb");
static BLEUUID alert_sev_uuid("00001802-0000-1000-8000-00805f9b34fb");
static BLEUUID alert_notify_sev_uuid("00001811-0000-1000-8000-00805f9b34fb");

// *** CHARACTERISTIC ***
// AUTH_CHAR
static BLEUUID auth_characteristic_uuid("00000009-0000-3512-2118-0009af100700");
// HRM_MEASURE
static BLEUUID UUID_CHAR_HRM_MEASURE("00002a37-0000-1000-8000-00805f9b34fb");
// HRM_CONTROL
static BLEUUID UUID_CHAR_HRM_CONTROL("00002a39-0000-1000-8000-00805f9b34fb");
// Not used
static BLEUUID alert_cha_uuid("00002a06-0000-1000-8000-00805f9b34fb");

// DES (all)
static BLEUUID CCCD_UUID((uint16_t)0x2902);

// *** COMMAND ***
static uint8_t HRM_ONESHOT_STOP[3]	= {0x15, 0x02, 0x00};
static uint8_t HRM_ONESHOT_START[3]	= {0x15, 0x02, 0x01};
static uint8_t HRM_CONTINUOUS_STOP[3]	= {0x15, 0x01, 0x00};
static uint8_t HRM_CONTINUOUS_START[3]	= {0x15, 0x01, 0x01};
static uint8_t HRM_NOTIFICATION[2]		= {0x01, 0x00};
static uint8_t HRM_HEARTBEAT[2]    = {0x16, 0x00};

/*
static uint8_t HRM_COMMAND			= 0x15;
static uint8_t HRM_MODE_SLEEP		= 0x00;
static uint8_t HRM_MODE_CONTINUOUS	= 0x01;
static uint8_t HRM_MODE_ONE_SHOT	= 0x02;
*/