#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <heltec.h>
#include <queue>
#include <unordered_set>
#include <ArduinoJson.h>
#include <string>
#include <math.h>

#define NODE0

#ifdef NODE0
#include "test/node0.h"
#else
#include "test/node1.h"
#endif

#define DBG_OUTPUT_PORT Serial
#define debug_mode true
#define BAND 433E6 // set band directly here: 433E6, 868E6,915E6
#define BAUD_RATE 115200
#define channel_num 1     // channel number for softAP
#define max_connection 10 // max connections to AP
#define send_delay 0      // wait before send
#define historySize 15
#define maxLoraPacket 200 // bytes in size

IPAddress Ip(192, 168, 1, 1); // ip address for website
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct Message
{
  std::string payload;
  byte sender;
  byte destination;
  byte id;
  byte type;
  int hash;
  Message(std::string _payload, byte _sender, byte _destination, byte _id, byte _type) : payload{_payload},
                                                                                         sender(_sender),
                                                                                         destination{_destination},
                                                                                         id{_id},
                                                                                         type{_type}
  {
    hash = std::hash<std::string>{}(payload + (char)sender + (char)destination + (char)id + (char)type);
  }
};

std::queue<Message> outgoingMessages;
std::queue<int *> hashQueue;
std::unordered_set<int> hashHistory;
static bool sending = false;
unsigned long lastSend = 0;
byte msgCount = 0; // count of outgoing messages

byte typeMessage = 0x00;

static const unsigned long BLINK_INTERVAL = 1000; //ms
static unsigned long lastBlink = 0;
static bool blinkState = false; // false = off

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return; // if there's no packet, return
  DBG_OUTPUT_PORT.println("received message");
  // read packet header bytes:
  byte recipient = LoRa.read();
  byte sender = LoRa.read();
  byte id = LoRa.read(); // incoming msg ID
  byte type = LoRa.read();
  byte length = LoRa.read(); // incoming msg length

  std::string payload = ""; // payload of packet

  while (LoRa.available())        // can't use readString() in callback
    payload += (char)LoRa.read(); // add bytes one by one

  if (debug_mode && length != payload.length()) // check length for error
  {
    DBG_OUTPUT_PORT.println("error: message length does not match length");
    return; // skip rest of function
  }
  Message message(payload, sender, recipient, id, type);
  if (recipient != address)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.println("this message is not for me.");
    if (hashHistory.find(message.hash) == hashHistory.end())
    {
      // not found in set
      outgoingMessages.push(message);
      hashHistory.insert(message.hash);
      hashQueue.push(&message.hash);
      if (hashQueue.size() > historySize)
      {
        hashHistory.erase(*hashQueue.front());
        hashQueue.pop();
      }
    }
    return;
  }
  if (debug_mode)
  {
    // if message is for this device, or broadcast, print details:
    DBG_OUTPUT_PORT.println("Message ID: " + String(id));
    DBG_OUTPUT_PORT.println("Message length: " + String(length));
    DBG_OUTPUT_PORT.println("Message: " + String(payload.c_str()));
    DBG_OUTPUT_PORT.println("RSSI: " + String(LoRa.packetRssi()));
  }
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, String(payload.c_str()) + ", " + String(id));
  Heltec.display->display();
}

void handleNotFound(AsyncWebServerRequest *request)
{
  // String path = request->url();
  String message = "\nNo Handler\r\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nParameters: ";
  message += request->params();
  message += "\n";
  for (uint8_t i = 0; i < request->params(); i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    String name = p->name().c_str();
    String val = p->value().c_str();
    message += (name + " : " + val + "\r\n");
  }
  request->send(404, "text/plain", message);
  if (debug_mode)
    DBG_OUTPUT_PORT.print(message);
}

void sendMessage()
{
  sending = true;
  Message message = outgoingMessages.front();
  LoRa.beginPacket();                          // start packet
  LoRa.write(message.destination);             // add message destination
  LoRa.write(address);                         // add local address
  LoRa.write(message.id);                      // add message ID
  LoRa.write(message.type);                    // add message type
  LoRa.write(message.payload.length());        // add payload length
  LoRa.print(String(message.payload.c_str())); // add payload
  LoRa.endPacket();                            // finish packet and send it
  msgCount++;                                  // increment message ID
  outgoingMessages.pop();
  sending = false;
  lastSend = millis();
}

void saveMessages(std::string fullmessage, byte destination, byte id, byte type)
{
  // split into multiple packets
  for (int i = 0; i < ceil((double)fullmessage.length() / maxLoraPacket); i++) {
    outgoingMessages.push(Message(fullmessage.substr(i * maxLoraPacket, maxLoraPacket), address, destination, id, type));
  }
}

void handleError(AsyncWebServerRequest *request, int errorCode, String errorMessage)
{
  DynamicJsonDocument errorObj(200);
  errorObj["message"] = errorMessage;
  String jsonError;
  serializeJson(errorObj, jsonError);
  request->send(errorCode, "application/json", jsonError.c_str());
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    if (debug_mode)
    {
      DBG_OUTPUT_PORT.printf("ws[%s][%u] connect\n", server->url(), client->id());
      client->ping();
    }
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    std::string msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      if (debug_mode)
        DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
          msg += (char)data[i];
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      if (debug_mode)
        DBG_OUTPUT_PORT.printf("%s\n", msg.c_str());
      if (debug_mode)
        DBG_OUTPUT_PORT.println(info->opcode == WS_TEXT ? "received text message" : "received binary message");
      client->text("received data");
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          if (debug_mode)
            DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        if (debug_mode)
          DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      if (debug_mode)
        DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
          msg += (char)data[i];
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      if (debug_mode)
        DBG_OUTPUT_PORT.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        if (debug_mode)
          DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          if (debug_mode)
          {
            DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
            DBG_OUTPUT_PORT.println(info->opcode == WS_TEXT ? "received text message" : "received binary message");
          }
        }
      }
    }
  }
}

void setup()
{
  DBG_OUTPUT_PORT.begin(BAUD_RATE);
  DBG_OUTPUT_PORT.setDebugOutput(debug_mode);
  lastSend = millis();
  hashHistory.reserve(historySize);
  WiFi.mode(WIFI_AP);
  if (debug_mode)
    DBG_OUTPUT_PORT.println("#Wait 100 ms for AP_START...");
  delay(100);
  WiFi.softAP(ssid, password, channel_num, 0, max_connection);
  if (debug_mode)
    DBG_OUTPUT_PORT.println("#Set softAPConfig");
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);

  IPAddress IP = WiFi.softAPIP();
  if (debug_mode)
  {
    DBG_OUTPUT_PORT.print("#IP address: ");
    DBG_OUTPUT_PORT.println(IP);
  }

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#hello get request");
    request->send(200, "text/plain", "Hello World Get");
  });

  server.on("/message", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    DBG_OUTPUT_PORT.println("#message put request");
    DynamicJsonDocument dataObj(200);
    DeserializationError error = deserializeJson(dataObj, data);
    if (error) {
      handleError(request, 500, error.c_str());
      return;
    }
    if (!dataObj.containsKey("message")) {
      handleError(request, 500, "no message key found");
      return;
    }
    String message = dataObj["message"];
    if (message == "") {
      handleError(request, 500, "no message found");
      return;
    }
    if (!dataObj.containsKey("destination")) {
      handleError(request, 500, "no destination key found");
      return;
    }
    byte messageDestination = dataObj["destination"];
    saveMessages(message.c_str(), messageDestination, msgCount, typeMessage);
    DynamicJsonDocument successObj(200);
    successObj["message"] = "sent message " + message;
    String successStr;
    serializeJson(successObj, successStr);
    request->send(200, "application/json", successStr.c_str()); });

  // add web socket stuff
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  //handle not found
  server.onNotFound(handleNotFound);
  if (debug_mode)
    DBG_OUTPUT_PORT.println("#finished creating server");
  server.begin();
  if (debug_mode)
    DBG_OUTPUT_PORT.println("#HTTP server started");

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

void loop()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize)
    onReceive(packetSize);
  if (millis() - lastBlink >= BLINK_INTERVAL)
  {
    lastBlink += BLINK_INTERVAL;
    blinkState = !blinkState;
    digitalWrite(LED_BUILTIN, blinkState);
  }
  if (!sending && millis() - lastSend > send_delay && !outgoingMessages.empty())
  {
    sendMessage();
  }
}
