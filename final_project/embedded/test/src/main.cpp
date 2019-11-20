#include <Arduino.h>
#include <heltec.h>

#define NODE1

#ifdef NODE0
#include "test/node0.h"
#else
#include "test/node1.h"
#endif

long last_time_send = 0;
String rssi = "RSSI --";
String packet = "";

void cbk(int packetSize)
{
  if (packetSize == 0)
    return;
  Serial.println("got packet");
  packet = "";
  while (LoRa.available())       // can't use readString() in callback
    packet += (char)LoRa.read(); // add bytes one by one
  Serial.println(packet);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, packet);
  Heltec.display->display();
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Heltec.begin(true, true, true, true, BAND);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, NAME);
  Heltec.display->display();
  last_time_send = millis();
  LoRa.onReceive(cbk);
  LoRa.receive();
  delay(5000);
}

int count = 0;

void loop()
{
  if (millis() - last_time_send > SEND_INTERVAL)
  {
    Serial.println("send");
    last_time_send = millis();
    /*
     * LoRa.setTxPower(txPower,RFOUT_pin);
     * txPower -- 0 ~ 20
     * RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
     *   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
     *   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
     */
    LoRa.beginPacket();
    LoRa.print(MESSAGE);
    LoRa.print(", count: ");
    LoRa.print(count);
    LoRa.endPacket();
    count++;
    LoRa.receive();
  }
  delay(10);
}
