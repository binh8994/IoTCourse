
#include <SoftwareSerial.h>
#include <Servo.h>
#include <RAK811.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define WORK_MODE LoRaWAN   //  LoRaWAN or LoRaP2P
#define JOIN_MODE ABP    //  OTAA or ABP

#define LoraSerial    Serial
#define TXpin         3
#define RXpin         2
#define LED           13
#define DS18B20_PIN   4
#define TURB_ADC      A0
#define SERVO         6
#define PUMP          5

struct {
  int16_t temp;
  int16_t turb;
} Lora_msg;

SoftwareSerial DBG(RXpin, TXpin);
Servo myservo;
RAK811 RAKLoRa(LoraSerial);
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

String NwkSKey = "3BF0A70FA7E2A809A7D1A9BCD505797B";
String AppSKey = "24B4464EA2E2A0FD5227ADA0B13F509B";
String DevAddr = "26041F84";

bool M2_On = false;
uint32_t M2_on_time;

void setup() {
  DBG.begin(115200);
  while (!DBG);
  while (DBG.read() >= 0) {}
  DBG.println("StartUP");

  pinMode(LED, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(SERVO, OUTPUT);
  digitalWrite(LED, LOW);
  digitalWrite(PUMP, LOW);

  LoraSerial.begin(115200);
  while (!LoraSerial);
  while (LoraSerial.read() >= 0) {}

  sensors.begin();

  pump(false);
  feed(false);
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

        //DBG.println("Get retrans: ");
        //DBG.println(RAKLoRa.rk_getConfig("retrans"));

        //DBG.println("Get rx2: ");
        //DBG.println(RAKLoRa.rk_getConfig("rx2"));

        //DBG.println("Get ch_list: ");
        //DBG.println(RAKLoRa.rk_getConfig("ch_list"));

        while (1) {
          readSensors();
          sendPkt();

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
                  down_link.remove(down_link.length() - 1);
                  parserCommand(&down_link);
                }
              }

              if (M2_On && ((uint32_t)millis() - M2_on_time) > 10000) {
                M2_On = false;
                digitalWrite(PUMP, LOW);
              }

            }
          }
          else {
            DBG.println("Send data fail!");
            break;
          }

          if (M2_On && ((uint32_t)millis() - M2_on_time) > 10000) {
            M2_On = false;
            digitalWrite(PUMP, LOW);
          }

        }
      }
    }
  }
  delay(1000);
  
  if (M2_On && ((uint32_t)millis() - M2_on_time) > 10000) {
    M2_On = false;
    digitalWrite(PUMP, LOW);
  }
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

void readSensors() {
  sensors.requestTemperatures();
  Lora_msg.temp = (int16_t)(sensors.getTempCByIndex(0) * 100);  //Temp x 100
  Lora_msg.turb = (int16_t)(analogRead(TURB_ADC) * 5.0 / 10.24);//Turb x 100
  DBG.print("Temp: "); DBG.println(Lora_msg.temp);
  DBG.print("Turb: "); DBG.println(Lora_msg.turb);
}

void sendPkt() {
  String send_str = String("at+send=0,1,") + arr2hexStr(&Lora_msg, sizeof(Lora_msg));
  //DBG.print("Send Data: ");
  DBG.println(send_str);
  LoraSerial.println(send_str);
  delay(500);
}

void pump(bool cmd) {
  if (cmd) {
    if (!M2_On) {
      M2_On = true;
      digitalWrite(PUMP, HIGH);
      M2_on_time = millis();
    }
    DBG.println("Pump:ON");
  }
  else {
    if (M2_On) {
      M2_On = false;
      digitalWrite(PUMP, LOW);
    }
    DBG.println("Pump:OFF");
  }
}

void feed(bool cmd) {
  myservo.attach(SERVO);
  delay(10);
  if (cmd) {
    myservo.write(140);
    delay(300);
    myservo.write(150);
    delay(300);
    DBG.println("Feed:ON");
    pinMode(SERVO, INPUT);
  }
  else {
    myservo.write(150);
    delay(300);
    DBG.println("Feed:OFF");
    pinMode(SERVO, INPUT);
  }
}

void parserCommand(String* payload) {
  if (payload->length() != 4) {
    DBG.println("Length err");
    return;
  }

  DBG.println(*payload);

  /*check type cmd:
    00 -> control pump
    01 -> control feed
  */
  if (payload->startsWith("00")) {
    if (payload->endsWith("00")) {
      pump(false);
    }
    else if (payload->endsWith("01")) {
      pump(true);
    }
  }

  else if (payload->startsWith("01")) {
    if (payload->endsWith("01")) {
      feed(true);
    }
    else if (payload->endsWith("01")) {
      feed(false);
    }
  }

  else {
    DBG.println("Parser err");
  }
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



