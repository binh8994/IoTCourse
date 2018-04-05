
#include <SoftwareSerial.h>
#include <RAK811.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define WORK_MODE LoRaWAN   //  LoRaWAN or LoRaP2P
#define JOIN_MODE ABP    //  OTAA or ABP

#define LoraSerial    Serial
#define TXpin         3
#define RXpin         2
#define LED           13
#define ONE_WIRE_BUS  4

#define SENSOR_TYPE   0
#define CONTROL_TYPE  1

struct {
  int8_t msg_type;
  int16_t temp;
  int16_t turb;
} Lora_msg;

SoftwareSerial DBG(RXpin, TXpin);
RAK811 RAKLoRa(LoraSerial);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

String NwkSKey = "3BF0A70FA7E2A809A7D1A9BCD505797B";
String AppSKey = "24B4464EA2E2A0FD5227ADA0B13F509B";
String DevAddr = "26041F84";
//char* buffer[32];

void setup() {
  DBG.begin(115200);
  while (!DBG);
  while (DBG.read() >= 0) {}
  DBG.println("StartUP");

  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  LoraSerial.begin(115200);
  while (!LoraSerial);
  while (LoraSerial.read() >= 0) {}

  sensors.begin();

  Lora_msg.msg_type = SENSOR_TYPE;
}

void loop() {

  //DBG.println(RAKLoRa.rk_getVersion());
  //delay(500);
  //DBG.println(RAKLoRa.rk_getBand());
  //delay(500);
  if (RAKLoRa.rk_setWorkingMode(WORK_MODE))
  {
    DBG.println("Set mode OK!");
    if (RAKLoRa.rk_initABP(DevAddr, NwkSKey, AppSKey))
    {
      DBG.println("Init ABP OK!");
      if (RAKLoRa.rk_joinLoRaNetwork(JOIN_MODE))
      {
        DBG.println("Join OK!");

        //DBG.println("Set retrans");
        RAKLoRa.rk_setConfig("retrans", "1");

        //DBG.println("Set dr");
        RAKLoRa.rk_setConfig("dr", "5");

        //DBG.println("Set tx_power");
        RAKLoRa.rk_setConfig("tx_power", "20");

        //DBG.println("Set duty");
        RAKLoRa.rk_setConfig("duty", "off");


        //          static bool setted = false;
        //          if( setted == false ) {
        //            for(int i = 8; i < 63; i++) {
        //              String index = String(i);
        //              String value = index + String(",off,0,3");
        //              RAKLoRa.rk_setConfig("ch_list", value);
        //              DBG.println(value);
        //              setted = true;
        //              delay(500);
        //            }
        //          }

        //DBG.println("Get retrans: ");
        //DBG.println(RAKLoRa.rk_getConfig("retrans"));

        //DBG.println("Get rx2: ");
        //DBG.println(RAKLoRa.rk_getConfig("rx2"));

        //DBG.println("Get ch_list: ");
        //DBG.println(RAKLoRa.rk_getConfig("ch_list"));

        while (1) {
          sensors.requestTemperatures();
          Lora_msg.temp = (int16_t)(sensors.getTempCByIndex(0) * 100);
          Lora_msg.turb = (int16_t)(rand());
          DBG.print("Temp: "); DBG.println(Lora_msg.temp);
          DBG.print("Turb: "); DBG.println(Lora_msg.turb);

          String send_str = String("at+send=0,1,") + arr2hexStr(&Lora_msg, sizeof(Lora_msg));
          DBG.print("Send Data: ");
          DBG.println(send_str);
          LoraSerial.println(send_str);
          delay(500);
          LoraSerial.setTimeout(2000);
          String recv = LoraSerial.readStringUntil('\n');
          if ( recv.indexOf("OK") != -1 ) {

            unsigned long time = millis();
            while ( (((unsigned long)millis() - time) < 60000) ) {

              String recv = LoraSerial.readStringUntil('\n');
              if (recv.length()) {
                DBG.print('[');  DBG.print(recv); DBG.println(']');

                if ( (recv.indexOf("=2,0,0") != -1) ) {
                  DBG.println("Send data OK");
                }
                else if ( (recv.indexOf("=0,1,") != -1) ) {
                  DBG.println("Recv something");
                  String down_link = recv.substring(recv.lastIndexOf(',') + 1);
                  //DBG.println(down_link.length());
                  //DBG.println(down_link);
                  if (down_link.length() == 7) {
                    if (down_link.startsWith("010000")) {
                      DBG.println("M1 OFF, M2 OFF");
                    }
                    else if (down_link.startsWith("010001")) {
                      DBG.println("M1 OFF, M2 ON");
                    }
                    else if (down_link.startsWith("010100")) {
                      DBG.println("M1 ON, M2 OFF");
                    }
                    else if (down_link.startsWith("010101")) {
                      DBG.println("M1 ON, M2 ON");
                    }
                    else {
                      DBG.println("Parser err");
                    }
                  }
                  else {
                    DBG.println("Parser err");
                  }

                }
              }
            }
          }
          else {
            DBG.println("Send data fail!");
            break;
          }
        }
      }
    }
  }
  delay(1000);
}

String arr2hexStr( void* in, int size_in) {
  String ret;
  char tmp[3];
  for (int i = 0; i < size_in; i++) {
    sprintf(tmp, "%02X", *((uint8_t*)in + i));
    ret += tmp;
  }
  return ret;
}

//at+mode=0
//at+set_config=dev_addr:2604195E&nwks_key:CBF42B80A3103CC4DF8471C23F90B0B9&apps_key:73AB8EAB4D5BD7EBFB34F8820C750140&tx_power:20&duty:off&retrans:1
////at+set_config=dev_addr:2604195E&nwks_key:01020304050607080910111213141516&apps_key:01020304050607080910111213141516&tx_power:20&duty:off&retrans:1
//at+set_config=ch_list:8,off&ch_list:9,off&ch_list:10,off&ch_list:11,off&ch_list:12,off&ch_list:13,off&ch_list:14,off&ch_list:15,off&ch_list:16,off&ch_list:17,off&ch_list:18,off&ch_list:19,off&ch_list:20,off&ch_list:21,off&ch_list:22,off&ch_list:23,off
//at+set_config=ch_list:24,off&ch_list:25,off&ch_list:26,off&ch_list:27,off&ch_list:28,off&ch_list:29,off&ch_list:30,off&ch_list:31,off&ch_list:32,off&ch_list:33,off&ch_list:34,off&ch_list:35,off&ch_list:36,off&ch_list:37,off&ch_list:38,off&ch_list:39,off
//at+set_config=ch_list:40,off&ch_list:41,off&ch_list:42,off&ch_list:43,off&ch_list:44,off&ch_list:45,off&ch_list:46,off&ch_list:47,off&ch_list:48,off&ch_list:49,off&ch_list:50,off&ch_list:51,off&ch_list:52,off&ch_list:53,off&ch_list:54,off&ch_list:55,off
//at+set_config=ch_list:56,off&ch_list:57,off&ch_list:58,off&ch_list:59,off&ch_list:60,off&ch_list:61,off&ch_list:62,off&ch_list:63,off
//at+join=abp
//at+send=1,1,72616B776972656C657373



