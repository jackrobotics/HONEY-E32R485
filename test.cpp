#include <Arduino.h>
#include "HONEY_AT_SIM7020E.h"
HONEY_AT_SIM7020E NB;

void callback(String &topic, String &payload, int &QoS, int &retained)
{
    Serial.println("topic : " + topic + " : " + payload);
}

void mqtt_setup()
{
    long _time = millis();
    int i = 0;
    while (i < 60 && millis() - _time < 30000)
    {
        i++;
        if (NB.MQTTConnect("mqtt_clientid", "mqtt_user", "mqtt_pass"))
            break;
        delay(100);
    }
    NB.MQTTSubscribe("/nb");
}

void setup()
{
    NB.begin(Serial2)
    NB.MQTTServer("43.229.135.169", 1883);
    NB.setCallbackMQTT(callback);
}

void loop()
{
    if (!NB.MQTTStatus())
        mqtt_setup();

    unsigned long lastupdate = millis();
    if (millis() - lastupdate > 1000)
    {
        lastupdate = millis();
        String msg = "Hello world";
        NB.MQTTPublish("mqtt_topic", msg);
    }
}
