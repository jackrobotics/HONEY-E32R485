#ifndef HONEY_AT_SIM7020E_H
#define HONEY_AT_SIM7020E_H

#include <Arduino.h>
#include <Stream.h>
#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#define hwResetPin 32

struct pingRESP
{
	bool status;
	String addr;
	String ttl;
	String rtt;
};

struct radio
{
	String pci = "";
	String rsrp = "";
	String rsrq = "";
	String snr = "";
};

struct dateTime
{
	String date = "";
	String time = "";
};

typedef void (*MQTTClientCallback)(String &topic, String &payload, int &QoS, int &retained);
typedef void (*reponseCallback)(String &payload);

class HONEY_AT_SIM7020E
{
public:
	HONEY_AT_SIM7020E();

	bool begin(Stream &);
	String CALL_AT_CMD(String cmd, unsigned long timeout = 5000);
	String CALL_AT_CMD_OK(String cmd, unsigned long timeout = 5000);
	String CALL_AT_CMD_FIND_LINE(String cmd, String str, unsigned long timeout = 5000);
	bool CALL_AT_CMD_FIND(String cmd, String str, unsigned long timeout = 5000);

	// --------- Parameter config ---------------
	const unsigned int mqttCmdTimeout = 1200;
	const unsigned int mqttBuffSize = 1024;

	// =========Initialization Module=======
	void setupModule(String port = "", String address = "");
	void reboot_module();
	pingRESP pingIP(String IP);
	bool NBstatus();
	bool attachNetwork();
	bool powerSavingMode(unsigned int psm);
	bool syncLocalTime();

	// ==========Get Parameter Value=========
	String getFirmwareVersion();
	String getIMEI();
	String getICCID();
	String getIMSI();
	String getDeviceIP();
	int getSignal();
	String getAPN();
	String getNetworkStatus();
	radio getRadioStat();
	bool checkPSMmode();
	bool MQTTStatus();
	dateTime getClock(unsigned int timezone = 7);

	// ==========Data send/rec.===============
	bool createUDPSocket(String address, unsigned int port);
	bool createTCPSocket(String address, unsigned int port);
	bool sendMSG(String);
	unsigned int loopSocket();
	bool closeSocket();

	// ================MQTT===================
	bool MQTTDisconnect();
	bool MQTTServer(String server, unsigned long port);
	bool MQTTConnect(String clientID, String username = "", String password = "", int keepalive = 600, int version = 3, int cleansession = 0, int willflag = 0, String willOption = "");
	bool MQTTPublish(String topic, String payload, unsigned int qos = 0, unsigned int retained = 0, unsigned int dup = 0);
	bool MQTTSubscribe(String topic, unsigned int qos = 0);
	bool MQTTUnsubscribe(String topic);
	unsigned int MQTTloop();

	// ============ callback ==================
	void setCallbackMQTT(MQTTClientCallback callbackFunc);
	void setCallback(reponseCallback callbackFunc);
	reponseCallback callback_p;
	MQTTClientCallback MQTTcallback_p;

	// ============ HTTP ==================
	void HTTPRequest(int method, String address, String url, String header, String content_type, String payload);
	void LINENotify(String, String);

private:
	// ==============Function==================
	bool setPhoneFunction();
	void connectNetwork();
	bool enterPIN();

	String str2hex(String);
	String hex2str(String);

protected:
	Stream *_Serial;
};

#endif