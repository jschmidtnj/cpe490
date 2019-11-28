#include <Arduino.h>
#include <heltec.h>

#define NODE0

#ifdef NODE0
#include "test/node0.h"
#else
#include "test/node1.h"
#endif

#define BAND 915E6 // set band directly here
#define SEND_INTERVAL 1000
#define BAUD_RATE 115200

void display(String data) {
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 0 , data);
  Heltec.display->display();
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available())             // can't use readString() in callback
  {
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length())   // check length for error
  {
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }
  // if message is for this device, or broadcast, print details:
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  display(incoming + ", " + String(incomingMsgId));
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
  // LoRa.onReceive(onReceive);
  // LoRa.receive();
}

long lastSendTime = 0;        // last send time
byte msgCount = 0;            // count of outgoing messages
int interval = SEND_INTERVAL;          // interval between sends

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void loop()
{
  if (millis() - lastSendTime > interval) {
    sendMessage(MESSAGE);
    Serial.println("Sending " + String(MESSAGE));
    lastSendTime = millis();            // timestamp the message
    interval = random(SEND_INTERVAL) + SEND_INTERVAL;
    // LoRa.receive();
  }
  int packetSize = LoRa.parsePacket();
  if (packetSize)
    onReceive(packetSize);
}
