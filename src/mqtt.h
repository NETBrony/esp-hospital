#ifndef MQTT_H
#define MQTT_H

//================ MQTT =================
const char* mqtt_server = "e53e2233d1a141699f1204a648c861c2.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "netbrony";
const char* mqtt_pass   = "Net_112233";

//================ ota topic =================
const char* topic_ota_update   = "ota/update";
const char* topic_ota_status   = "ota/status";
//============================================

//=========== general Topic =============
const char* topic_TempHumi     = "sensor/TempHumi";
// const char* topic_error        = "sensor/error";
const char* topic_light_status = "light/status";
const char* topic_control      = "control/pump";

#endif