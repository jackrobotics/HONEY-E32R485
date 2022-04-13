#include "HONEY_AT_SIM7020E.h"
// #include "board.h"

/****************************************/
/**        Initialization Module       **/
/****************************************/
HONEY_AT_SIM7020E::HONEY_AT_SIM7020E() {}

String HONEY_AT_SIM7020E::CALL_AT_CMD(String cmd, unsigned long timeout)
{
	_Serial->flush();
	unsigned long _time = millis();
	String data = "";
	_Serial->println(cmd);
	while (millis() - _time < timeout)
	{
		if (_Serial->available())
		{
			delay(10);
			data = _Serial->readStringUntil('\n');
			data.trim();
			if (data.length() != 0)
				break;
		}
	}
	if (millis() - _time > timeout)
		data = "TIMEOUT";
	return data;
}

bool HONEY_AT_SIM7020E::CALL_AT_CMD_FIND(String cmd, String str, unsigned long timeout)
{
	_Serial->flush();
	String data = "";
	_Serial->println(cmd);
	delay(10);
	unsigned long _time = millis();
	while (millis() - _time < timeout)
	{
		while (_Serial->available())
		{
			char ch = _Serial->read();
			data += ch;
		}
		if (data.indexOf(str) != -1)
		{
			return true;
		}
	}
	if (millis() - _time > timeout)
		data = "TIMEOUT";
	return false;
}

String HONEY_AT_SIM7020E::CALL_AT_CMD_FIND_LINE(String cmd, String str, unsigned long timeout)
{
	_Serial->flush();
	String data = "";
	_Serial->println(cmd);
	unsigned long _time = millis();
	while (millis() - _time < timeout)
	{
		while (_Serial->available())
		{
			data = _Serial->readStringUntil('\n');
			data.trim();
		}
		if (data.indexOf(str) != -1)
		{
			return data;
		}
	}
	if (millis() - _time > timeout)
		data = "TIMEOUT";
	return data;
}

String HONEY_AT_SIM7020E::CALL_AT_CMD_OK(String cmd, unsigned long timeout)
{
	String data = "";
	_Serial->flush();
	_Serial->println(cmd);
	delay(10);
	unsigned long _time = millis();
	while (millis() - _time < timeout)
	{
		while (_Serial->available())
		{
			char ch = _Serial->read();
			data += ch;
		}
		if (data.indexOf(F("OK")) != -1)
		{
			data.replace(F("OK"), "");
			data.trim();
			break;
		}
	}
	if (millis() - _time > timeout)
		data = "TIMEOUT";
	return data;
}

bool HONEY_AT_SIM7020E::begin(Stream &serial)
{
	_Serial = &serial;

	// hardware reboot
	pinMode(hwResetPin, OUTPUT);
	DEBUG_PRINT(">> reboot module...");
	this->reboot_module();
	DEBUG_PRINTLN("OK");

	// echo off
	DEBUG_PRINT(">> echo mode off...");
	DEBUG_PRINTLN(this->CALL_AT_CMD_FIND_LINE("ATE0", "OK"));

	// loop check module
	DEBUG_PRINT(">> check module...");
	int loop_check_module = 0;
	for (int i = 0; i < 3; i++)
		if (CALL_AT_CMD_FIND("AT", "OK", 1000))
			loop_check_module++;
	if (loop_check_module)
		DEBUG_PRINTLN("OK");
	else
	{
		DEBUG_PRINTLN("ERROR");
		return false;
	}

	DEBUG_PRINT(">> Setup");
	this->CALL_AT_CMD_FIND("AT+CMEE=1", "OK"); // set report error
	this->CALL_AT_CMD_FIND("AT+CLTS=1", "OK"); // sync local time
	powerSavingMode(0);
	DEBUG_PRINTLN("...OK");

	DEBUG_PRINT(F(">> IMSI   : "));
	DEBUG_PRINTLN(this->getIMSI());

	DEBUG_PRINT(F(">> ICCID  : "));
	DEBUG_PRINTLN(this->getICCID());

	DEBUG_PRINT(F(">> IMEI   : "));
	DEBUG_PRINTLN(this->getIMEI());

	DEBUG_PRINT(F(">> FW ver : "));
	DEBUG_PRINTLN(getFirmwareVersion());

	DEBUG_PRINT(F(">> PSM mode : "));
	DEBUG_PRINTLN(checkPSMmode());

	//   delay(500);
	int _signal = -113;
	DEBUG_PRINT(F(">> Signal : "));
	for (int i = 0; i < 10; i++)
	{
		_signal = getSignal();
		DEBUG_PRINT(".");
	}
	DEBUG_PRINT(_signal);
	DEBUG_PRINTLN(F(" dBm"));
	if (_signal == -113)
		return false;

	//   _serial_flush();
	DEBUG_PRINT(F(">> Connecting."));

	unsigned long _time = millis();
	unsigned long _timeout = 10000;
	bool _attachNetwork = false;
	while (millis() - _time < _timeout)
	{
		if (attachNetwork())
		{
			_attachNetwork = true;
			DEBUG_PRINTLN("OK");
			break;
		}
		else
			DEBUG_PRINT(".");
		delay(200);
	}
	if (!_attachNetwork)
	{
		DEBUG_PRINTLN("TIMEOUT");
		return false;
	}
	delay(100);
	DEBUG_PRINT(F(">> NetworkStatus : "));
	String network_status = getNetworkStatus();
	DEBUG_PRINTLN(network_status);
	if (network_status != "Registered")
		return false;
	delay(100);
	DEBUG_PRINT(F(">> IP : "));
	DEBUG_PRINTLN(getDeviceIP());
	// pingIP("43.229.135.169");
	// getClock();
	// enterPIN();
	// DEBUG_PRINT(F(">> APN   : "));
	// DEBUG_PRINTLN(getAPN());
	return true;
}

//  Reboot module with hardware pin.
void HONEY_AT_SIM7020E::reboot_module()
{
	this->CALL_AT_CMD_FIND("AT+CPOWD=1", "OK");
	digitalWrite(hwResetPin, LOW);
	delay(1000);
	digitalWrite(hwResetPin, HIGH);
	delay(2000);
}

bool HONEY_AT_SIM7020E::attachNetwork()
{
	setPhoneFunction();
	connectNetwork();
	if (NBstatus())
		return true;
	else
		return false;
}

// // Check network connecting status : 1 connected, 0 not connected
bool HONEY_AT_SIM7020E::NBstatus()
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CGATT?", "+CGATT");
	if (data.indexOf("+CGATT: 1") != -1)
		return true;
	else
		return false;
}

// // Set Phone Functionality : 1 Full functionality
bool HONEY_AT_SIM7020E::setPhoneFunction()
{
	return this->CALL_AT_CMD_FIND_LINE("AT+CFUN=1", "OK");
}

// Attach network : 1 connected, 0 disconnected
void HONEY_AT_SIM7020E::connectNetwork()
{
	this->CALL_AT_CMD_FIND_LINE("AT+CGATT=1", "OK");
}

// Create a UDP socket and connect socket to remote address and port
bool HONEY_AT_SIM7020E::createUDPSocket(String address, unsigned int port)
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CSOC=1,2,1", "+CSOC:");
	if (data.indexOf(F("+CSOC: 0")) != -1)
	{
		return this->CALL_AT_CMD_FIND("AT+CSOCON=0," + String(port) + ",\"" + address + "\"", "OK");
	}
	else if (data.indexOf(F("+CSOC: 1")) != -1)
	{
		closeSocket();
		return false;
	}
	return false;
}

// Create a TCP socket and connect socket to remote address and port
bool HONEY_AT_SIM7020E::createTCPSocket(String address, unsigned int port)
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CSOC=1,1,1", "+CSOC:");
	if (data.indexOf(F("+CSOC: 0")) != -1)
	{
		return this->CALL_AT_CMD_FIND("AT+CSOCON=0," + String(port) + ",\"" + address + "\"", "OK");
	}
	else if (data.indexOf(F("+CSOC: 1")) != -1)
	{
		closeSocket();
		return false;
	}
	return false;
}

// Close a UDP socket 0
bool HONEY_AT_SIM7020E::closeSocket()
{
	return this->CALL_AT_CMD_FIND("AT+CSOCL=0", "OK");
}

// Ping IP
pingRESP HONEY_AT_SIM7020E::pingIP(String IP)
{
	pingRESP pingr;
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CIPPING=" + IP, "+CIPPING:");
	Serial.println(data);
	if (data.indexOf("+CIPPING:") != -1)
	{
		int index = data.indexOf(",") + 1;
		pingr.addr = data.substring(index, data.indexOf(",", index));
		index = data.indexOf(",", index) + 1;
		pingr.rtt = String(data.substring(index, data.indexOf(",", index)).toInt() / 4 * 100);
		index = data.indexOf(",", index) + 1;
		pingr.ttl = String(data.substring(index, data.indexOf(",", index)).toInt() / 4);
		pingr.status = true;
	}
	else
	{
		pingr.status = false;
	}
	return pingr;
}

// Set powerSavingMode : 0 turn off, 1 turn on
bool HONEY_AT_SIM7020E::powerSavingMode(unsigned int psm)
{
	return this->CALL_AT_CMD_FIND("AT+CPSMS=" + (String)psm, "OK");
}

// Check if SIM/eSIM need PIN or not.
bool HONEY_AT_SIM7020E::enterPIN()
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CPIN?", "+CPIN");
	if (data.indexOf(F("READY")) != -1)
		return true;
	else
		return false;
}

// /****************************************/
// /**          Get Parameter Value       **/
// /****************************************/
String HONEY_AT_SIM7020E::getIMSI()
{
	return this->CALL_AT_CMD_OK("AT+CIMI");
}

String HONEY_AT_SIM7020E::getICCID()
{
	return this->CALL_AT_CMD_OK("AT+CCID");
}

String HONEY_AT_SIM7020E::getIMEI()
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CGSN=1", "+CGSN:");
	data.replace(F("+CGSN: "), "");
	data.trim();
	return data;
}

String HONEY_AT_SIM7020E::getDeviceIP()
{
	for (int i = 0; i < 3; i++)
	{
		String data = this->CALL_AT_CMD_FIND_LINE("AT+CGPADDR=1", "+CGPADDR");
		if (data.indexOf(F("+CGPADDR")) != -1)
		{
			data = data.substring(data.indexOf(",") + 1);
			data.replace(F("\""), "");
			return data;
		}
	}
	return "N/A";
}

int HONEY_AT_SIM7020E::getSignal()
{
	int rssi = 0;
	int _time = millis();
	while (millis() - _time < 1000)
	{
		String data = this->CALL_AT_CMD_OK("AT+CSQ");
		String data_csq = data.substring(data.indexOf(F(":")) + 1, data.indexOf(F(";")));
		rssi = data_csq.toInt();
		rssi = (2 * rssi) - 113;
		if (rssi != -113 && rssi != 85)
			break;
	}
	if (rssi == -113 || rssi == 85)
		rssi = -113;
	return rssi;
}

String HONEY_AT_SIM7020E::getAPN()
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CGDCONT?", "+CGDCONT");
	if (data.indexOf("+CGDCONT") != -1)
	{
		int index = data.indexOf(",", data.indexOf(",") + 1) + 1;
		data = data.substring(index, data.indexOf(",", index));
		data.replace("\"", "");
	}
	return data;
}

String HONEY_AT_SIM7020E::getFirmwareVersion()
{
	for (int i = 0; i < 3; i++)
	{
		String data = this->CALL_AT_CMD_OK("AT+CGMR");
		if (data.length())
			return data;
	}
	return "N/A";
}

String HONEY_AT_SIM7020E::getNetworkStatus()
{
	this->CALL_AT_CMD_OK("AT+CEREG=2");
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CEREG?", "+CEREG");
	if (data.indexOf("+CEREG") != -1)
	{
		int index = data.indexOf(",", data.indexOf(",")) + 1;
		data = data.substring(index, data.indexOf(",", index));

		if (data == "1")
			data = "Registered";
		else if (data == "2")
			data = "Not Registered";
		else
			data = "Trying";
	}
	return data;
}

// // Get radio stat.
radio HONEY_AT_SIM7020E::getRadioStat()
{
	radio value;
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CENG?", "+CENG");
	if (data.indexOf("+CENG") != -1)
	{
		int index = data.indexOf(",", data.indexOf(",") + 1) + 1;
		value.pci = data.substring(index, data.indexOf(",", index));
		index = data.indexOf(",", index) + 1;
		index = data.indexOf(",", index) + 1;
		value.rsrp = data.substring(index, data.indexOf(",", index));
		index = data.indexOf(",", index) + 1;
		value.rsrq = data.substring(index, data.indexOf(",", index));
		index = data.indexOf(",", index) + 1;
		index = data.indexOf(",", index) + 1;
		value.snr = data.substring(index, data.indexOf(",", index));
	}
	return value;
}

bool HONEY_AT_SIM7020E::checkPSMmode()
{
	bool status = false;
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CPSMS?", "+CPSMS");
	if (data.indexOf(F("+CPSMS: ")) != -1)
	{
		if (data.indexOf(F("1")) != -1)
			status = true;
		else
			status = false;
	}
	return status;
}

/****************************************/
/**          Send TCP/UDP Message      **/
/****************************************/
// Send AT command to send TCP/UDP message
bool HONEY_AT_SIM7020E::sendMSG(String payload)
{
	return this->CALL_AT_CMD_FIND("AT+CSOSEND=0,0,\"" + payload + "\"", "OK");
}

/****************************************/
/**        Receive TCP/UDP Message     **/
/****************************************/
// Receive incoming message : +CSONMI: <socket_id>,<data_len>,<data>
unsigned int HONEY_AT_SIM7020E::loopSocket()
{
	String data = "";
	if (_Serial->available())
	{
		delay(10);
		data = _Serial->readStringUntil('\n');
	}
	if (data.indexOf(F("+CSONMI:")) != -1)
	{
		data.replace(F("+CSONMI:"), "");
		int index = data.indexOf(",") + 1;
		int len = data.substring(index, data.indexOf(",", index)).toInt();
		String hex = data.substring(data.indexOf(",", index) + 1);
		data = this->hex2str(hex);
		if (callback_p != NULL)
		{
			callback_p(data);
		}
		return len;
	}
	return 0;
}

dateTime HONEY_AT_SIM7020E::getClock(unsigned int timezone)
{
	dateTime dateTime;
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CCLK?", "CCLK");
	if (data.indexOf("+CCLK") != -1)
	{
		data.replace("+CCLK: ", "");
		dateTime.date = data.substring(0, data.indexOf(","));
		dateTime.time = data.substring(data.indexOf(",") + 1, data.indexOf("+"));

		int index = dateTime.date.indexOf("/");
		unsigned int yy = ("20" + dateTime.date.substring(0, index)).toInt();
		unsigned int mm = dateTime.date.substring(index + 1, dateTime.date.indexOf("/", index + 1) + 1).toInt();
		index = dateTime.date.indexOf("/", index + 1);
		unsigned int dd = dateTime.date.substring(index + 1).toInt();

		unsigned int hr = dateTime.time.substring(0, dateTime.time.indexOf(":")).toInt();
		hr += timezone;

		if (hr >= 24)
		{
			hr -= 24;
			//date+1
			dd += 1;
			if (mm == 2)
			{
				if ((((yy % 4 == 0) && (yy % 100 != 0)) || yy % 400 == 0))
				{
					if (dd > 29)
					{
						dd = 1;
						mm += 1;
					}
				}
				else if (dd > 28)
				{
					dd = 1;
					mm += 1;
				}
			}
			else if ((mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) && dd > 31)
			{
				dd = 1;
				mm += 1;
			}
			else if (dd > 30)
			{
				dd = 1;
				mm += 1;
			}
		}
		dateTime.date = String(yy) + "/" + ((mm < 10) ? "0" : "") + String(mm) + "/" + ((dd < 10) ? "0" : "") + String(dd);
		dateTime.time = ((hr < 10) ? "0" : "") + String(hr) + dateTime.time.substring(dateTime.time.indexOf(":"));
	}
	return dateTime;
}

bool HONEY_AT_SIM7020E::syncLocalTime()
{
	return this->CALL_AT_CMD_FIND_LINE("AT+CLTS=1", "+CLTS");
}

/****************************************/
/**                MQTT                **/
/****************************************/

bool HONEY_AT_SIM7020E::MQTTServer(String server, unsigned long port)
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CMQNEW=\"" + server + "\",\"" + String(port) + "\"," + String(mqttCmdTimeout) + "," + String(mqttBuffSize), "+CMQNEW:");
	if (data.indexOf("OK") != -1)
		return true;
	else
		return false;
}

bool HONEY_AT_SIM7020E::MQTTConnect(String clientID, String username, String password, int keepalive, int version, int cleansession, int willflag, String willOption)
{
	//AT+CMQCON=<mqtt_id>,<version>,<client_id>,<keepalive_interval>,<cleansession>,<will_flag>[,<will_options>][,<username>,<password>]
	String cmd = "AT+CMQCON=0," + String(version) + ",\"" + clientID + "\"," + String(keepalive) + "," + String(cleansession) + "," + String(willflag);
	if (willflag)
	{
		cmd += willOption;
	}
	if (username.length() > 0)
	{
		cmd += ",\"" + username + "\",\"" + password + "\"";
	}
	return this->CALL_AT_CMD_FIND(cmd, "OK");
}

bool HONEY_AT_SIM7020E::MQTTDisconnect()
{
	return this->CALL_AT_CMD_FIND("AT+CMQDISCON=0", "OK");
}

bool HONEY_AT_SIM7020E::MQTTStatus()
{
	String data = this->CALL_AT_CMD_FIND_LINE("AT+CMQCON?", "+CMQCON:");
	if (data.indexOf("+CMQCON:") != -1)
	{
		data = data.substring(data.indexOf(",") + 1, data.indexOf(",", data.indexOf(",") + 1));
		if (data.indexOf("1") != -1)
			return true;
	}
	return false;
}

bool HONEY_AT_SIM7020E::MQTTPublish(String topic, String payload, unsigned int qos, unsigned int retained, unsigned int dup)
{
	//AT+CMQPUB=<mqtt_id>,<topic>,<QoS>,<retained>,<dup>,<message_len>,<message>
	String hex = this->str2hex(payload);
	return this->CALL_AT_CMD_FIND("AT+CMQPUB=0,\"" + topic + "\"," + String(qos) + "," + String(retained) + "," + String(dup) + "," + String(payload.length() * 2) + ",\"" + hex + "\"", "OK");
}

bool HONEY_AT_SIM7020E::MQTTSubscribe(String topic, unsigned int qos)
{
	//AT+CMQSUB=<mqtt_id>,<topic>,<QoS>
	return this->CALL_AT_CMD_FIND("AT+CMQSUB=0,\"" + topic + "\"," + String(qos), "OK");
}

bool HONEY_AT_SIM7020E::MQTTUnsubscribe(String topic)
{
	return this->CALL_AT_CMD_FIND("AT+CMQUNSUB=0,\"" + topic + "\"", "OK");
}

unsigned int HONEY_AT_SIM7020E::MQTTloop()
{ //clear buffer before this
	String data = "";
	if (_Serial->available())
	{
		delay(10);
		data = _Serial->readStringUntil('\n');
	}
	if (data.indexOf(F("+CMQPUB:")) != -1)
	{
		data.replace(F("+CMQPUB:"), "");
		int index = data.indexOf(",") + 1;
		String topic = data.substring(index, data.indexOf(",", index));
		topic.replace("\"", "");
		// Serial.println(topic);
		index = data.indexOf(",", index) + 1;
		int qos = data.substring(index, data.indexOf(",", index)).toInt();
		// Serial.println(qos);
		index = data.indexOf(",", index) + 1;
		int retained = data.substring(index, data.indexOf(",", index)).toInt();
		// Serial.println(retained);
		index = data.indexOf(",", index) + 1;
		// int duplicate = data.substring(index,data.indexOf(",",index)).toInt();
		// Serial.println(duplicate);
		index = data.indexOf(",", index) + 1;
		int len = data.substring(index, data.indexOf(",", index)).toInt();
		// Serial.println(len);
		index = data.indexOf(",", index) + 1;
		String hex = data.substring(index, data.indexOf(",", index));
		// Serial.println(hex);
		hex.replace("\"", "");

		data = this->hex2str(hex);

		if (this->MQTTcallback_p != NULL)
		{
			this->MQTTcallback_p(topic, data, qos, retained);
		}
		return len;
	}
	return 0;
}

void HONEY_AT_SIM7020E::setCallbackMQTT(MQTTClientCallback callbackFunc)
{
	MQTTcallback_p = callbackFunc;
}

void HONEY_AT_SIM7020E::setCallback(reponseCallback callbackFunc)
{
	callback_p = callbackFunc;
}

String HONEY_AT_SIM7020E::hex2str(String hex)
{
	String str = "";
	for (uint16_t i = 0; i < hex.length(); i += 2)
	{
		str += (char)strtol(hex.substring(i, i + 2).c_str(), NULL, 16);
	}
	return str;
}
String HONEY_AT_SIM7020E::str2hex(String payload)
{
	String hex = "";
	for (int i = 0; i < payload.length(); i++)
	{
		String ch = String(payload.charAt(i), HEX);
		if (ch.length() < 2)
			ch = "0" + ch;
		hex += ch;
	}
	return hex;
}