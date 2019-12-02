#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <heltec.h>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <ArduinoJson.h>
#include <string>
#include <math.h>
#include <vector>
#include <algorithm>

#define NODE0

#ifdef NODE0
#include "config/node0.h"
#else
#include "config/node1.h"
#endif

#define DBG_OUTPUT_PORT Serial
#define debug_mode true
#define verbose true
#define BAND 433E6 // set band directly here: 433E6, 868E6,915E6
#define BAUD_RATE 115200
#define channel_num 1     // channel number for softAP
#define max_connection 10 // max connections to AP
#define send_delay 0      // wait before send
#define historySize 15
#define maxLoraPacket 200    // bytes in size
#define senderHistorySize 20 // max sender history
#define create_local_host true
#define ttl 300 // ttl for dns
const char *host = "stormnet.com";

DNSServer dnsServer;
const byte DNS_PORT = 53;
IPAddress Ip(192, 168, 1, 1); // ip address for website
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct Message
{
  std::string payload;
  byte destination;
  byte sender;
  byte id;             // message send count / id
  byte frame;          // frame number
  byte packet;         // packet count / number
  byte packetLocation; // packet start, mid, or end
  byte type;
  int hash;
  Message(std::string _payload, byte _destination, byte _sender, byte _id, byte _frame, byte _packet, byte _packetLocation, byte _type) : payload{_payload},
                                                                                                                                          destination{_destination},
                                                                                                                                          sender(_sender),
                                                                                                                                          id{_id},
                                                                                                                                          frame{_frame},
                                                                                                                                          packet{_packet},
                                                                                                                                          packetLocation{_packetLocation},
                                                                                                                                          type{_type}
  {
    hash = std::hash<std::string>{}(payload + (char)sender + (char)destination + (char)id + (char)frame + (char)packet + (char)type);
  }
};

std::vector<AsyncWebSocketClient *> websocketClients;
std::queue<Message> outgoingMessages;
std::queue<int *> hashQueue;
std::unordered_set<int> hashHistory;
static bool sending = false;
unsigned long lastSend = 0;
byte msgCount = 0; // count of outgoing messages
std::vector<Message> incomingMessages;
std::unordered_map<AsyncWebSocketClient *, std::vector<byte>> messageHeaders;
std::unordered_set<byte> pastSenders;
std::queue<byte *> pastSendersQueue;

byte startType = 0xF0;
byte endType = 0x0F;
byte midType = 0x00;
byte startEndType = 0xFF;
byte typeMessage = 0x00;

static const unsigned long PRINT_INTERVAL = 1000; //ms
static unsigned long lastPrint = 0;

static const unsigned long BLINK_INTERVAL = 1000; //ms
static unsigned long lastBlink = 0;
static bool blinkState = false; // false = off

void printStations()
{
  wifi_sta_list_t stationList;
  esp_wifi_ap_get_sta_list(&stationList);
  char headerChar[50] = {0};
  char buffer[5] = {0};
  String stationNumStr = itoa(stationList.num, buffer, 10);
  String headerStr = "Num Connect: " + stationNumStr;
  headerStr.toCharArray(headerChar, 50);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, headerChar);
  for (int i = 0; i < stationList.num; i++)
  {
    wifi_sta_info_t station = stationList.sta[i];
    String mac = "";
    for (int j = 0; j < 6; j++)
    {
      mac += (char)station.mac[j];
      if (j < 5)
        mac += ":";
    }
    if (debug_mode)
      DBG_OUTPUT_PORT.println(mac);
    char macChar[25];
    mac.substring(1, 16).toCharArray(macChar, 25);
    // Heltec.display->drawString(0, i + 1, macChar);
  }
  for (int i = 0; i < 7 - stationList.num; i++)
    Heltec.display->drawString(0, i + 2 + stationList.num, "                ");
  Heltec.display->display();
}

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return; // if there's no packet, return
  DBG_OUTPUT_PORT.println("received message");
  // read packet header bytes:
  byte recipient = LoRa.read();
  byte sender = LoRa.read();
  byte id = LoRa.read(); // incoming msg ID
  byte frame = LoRa.read();
  byte packet = LoRa.read();
  byte packetLocation = LoRa.read();
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
  Message message(payload, recipient, sender, id, frame, packet, packetLocation, type);
  if (recipient != address)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.println("this message is not for me. It's for " + String(recipient));
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
  // here the packet is for me
  incomingMessages.push_back(message);
  pastSenders.insert(sender);
  pastSendersQueue.push(&sender);
  if (pastSendersQueue.size() > senderHistorySize)
  {
    pastSenders.erase(*pastSendersQueue.front());
    pastSendersQueue.pop();
  }
  if (message.packetLocation == endType || message.packetLocation == startEndType)
  {
    std::string fullMessage = "";
    for (Message &message : incomingMessages)
      fullMessage.append(message.payload);
    std::string key = message.type == typeMessage ? "message" : "unknown";
    for (AsyncWebSocketClient *client : websocketClients)
    {
      DynamicJsonDocument messageObj(200);
      messageObj[key.c_str()] = fullMessage.c_str();
      String messageStr;
      serializeJson(messageObj, messageStr);
      client->text(messageStr.c_str());
    }
    incomingMessages.clear();
  }
  if (debug_mode)
  {
    // if message is for this device, or broadcast, print details:
    DBG_OUTPUT_PORT.println("Message ID: " + String(id));
    DBG_OUTPUT_PORT.println("Message length: " + String(length));
    DBG_OUTPUT_PORT.println("Message: " + String(payload.c_str()));
    DBG_OUTPUT_PORT.println("RSSI: " + String(LoRa.packetRssi()));
  }
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
  LoRa.write(message.frame);                   // add message frame
  LoRa.write(message.packet);                  // add packet number
  LoRa.write(message.packetLocation);          // add packet location type
  LoRa.write(message.type);                    // add message type
  LoRa.write(message.payload.length());        // add payload length
  LoRa.print(String(message.payload.c_str())); // add payload
  LoRa.endPacket();                            // finish packet and send it
  msgCount++;                                  // increment message ID
  outgoingMessages.pop();
  sending = false;
  lastSend = millis();
}

void saveMessages(std::string fullmessage, byte destination, byte id, byte frame, byte packetLocation, byte type)
{
  // split into multiple packets
  int endCount = ceil((double)fullmessage.length() / maxLoraPacket);
  for (int i = 0; i < endCount; i++)
  {
    byte location = midType;
    if (i == 0 || i == endCount - 1)
      location = packetLocation;
    outgoingMessages.push(Message(fullmessage.substr(i * maxLoraPacket, maxLoraPacket), destination, address, id, frame, i, location, type));
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
    if (debug_mode && verbose)
    {
      DBG_OUTPUT_PORT.printf("ws[%s][%u] connect\n", server->url(), client->id());
      client->ping();
    }
    websocketClients.push_back(client);
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    if (debug_mode && verbose)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    // delete from array
    websocketClients.erase(std::remove(websocketClients.begin(), websocketClients.end(), client), websocketClients.end());
  }
  else if (type == WS_EVT_ERROR)
  {
    if (debug_mode && verbose)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    if (debug_mode && verbose)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    std::string msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      if (debug_mode && verbose)
        DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      byte destination;
      byte type;
      if (info->opcode == WS_TEXT)
      {
        destination = data[0];
        type = data[1];
        for (size_t i = 2; i < info->len; i++)
          msg += (char)data[i];
      }
      else
      {
        if (debug_mode && verbose)
          DBG_OUTPUT_PORT.println("received binary data");
        return;
      }

      saveMessages(msg, destination, msgCount, 0, startEndType, type);
      if (debug_mode && verbose)
      {
        DBG_OUTPUT_PORT.println("send to " + String(destination));
        DBG_OUTPUT_PORT.printf("%s\n", msg.c_str());
        DBG_OUTPUT_PORT.println("received text message");
      }
      DynamicJsonDocument successObj(200);
      successObj["debug"] = "sent message";
      String successStr;
      serializeJson(successObj, successStr);
      client->text(successStr.c_str());
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      byte frameType = midType;
      if (info->index == 0)
      {
        if (info->num == 0)
          if (debug_mode && verbose)
            DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        if (debug_mode && verbose)
          DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
        frameType = startType;
      }

      if (debug_mode && verbose)
        DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      byte destination;
      byte type;
      size_t startIndex;
      if (frameType == startType)
      {
        destination = data[0];
        type = data[1];
        messageHeaders[client].push_back(destination);
        messageHeaders[client].push_back(type);
        startIndex = 2;
      }
      else
      {
        if (messageHeaders.find(client) == messageHeaders.end())
        {
          if (debug_mode)
            DBG_OUTPUT_PORT.println("could not find client in map");
          return;
        }
        destination = messageHeaders[client][0];
        destination = messageHeaders[client][1];
        startIndex = 0;
      }
      if (info->opcode == WS_TEXT)
      {
        for (size_t i = startIndex; i < len; i++)
          msg += (char)data[i];
      }
      else
      {
        if (debug_mode && verbose)
          DBG_OUTPUT_PORT.println("received binary data");
        return;
      }

      if (debug_mode && verbose)
        DBG_OUTPUT_PORT.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        if (debug_mode && verbose)
          DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          if (debug_mode && verbose)
          {
            DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
            DBG_OUTPUT_PORT.println("received text message");
          }
          frameType = endType;
        }
      }
      if (frameType == endType && messageHeaders.find(client) != messageHeaders.end())
      {
        messageHeaders.erase(client);
      }
      saveMessages(msg, destination, msgCount, info->index, frameType, type);
      DynamicJsonDocument successObj(200);
      successObj["debug"] = "sent message";
      String successStr;
      serializeJson(successObj, successStr);
      client->text(successStr.c_str());
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

  if (create_local_host)
  {
    //dnsServer.start(DNS_PORT, "*", IP);
    dnsServer.setTTL(ttl);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    dnsServer.start(DNS_PORT, host, IP);
  }

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#hello get request");
    request->send(200, "text/plain", "Hello World Get");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#config get request");
    DynamicJsonDocument configObj(200);
    configObj["source"] = String(address);
    String dataStr;
    serializeJson(configObj, dataStr);
    request->send(200, "application/json", dataStr.c_str()); });

  server.on("/senders", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#senders get request");
    DynamicJsonDocument sendersObj(200);
    JsonArray clients = sendersObj.createNestedArray("senders");
    for (const byte & sender : pastSenders) {
      if (debug_mode && verbose)
        DBG_OUTPUT_PORT.println(sender);
      clients.add(String(sender));
    }
    String dataStr;
    serializeJson(sendersObj, dataStr);
    request->send(200, "application/json", dataStr.c_str()); });

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
    saveMessages(message.c_str(), messageDestination, msgCount, 0, startEndType, typeMessage);
    DynamicJsonDocument successObj(200);
    successObj["message"] = "sent message";
    String successStr;
    serializeJson(successObj, successStr);
    request->send(200, "application/json", successStr.c_str()); });

  // add CORS
  server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Access-Control-Allow-Methods", "GET,PUT,POST,DELETE");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type,Access-Control-Allow-Headers,Authorization");
    request->send(response); });
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // add file handler
  SPIFFS.begin();
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

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
  if (create_local_host)
    dnsServer.processNextRequest();
  int packetSize = LoRa.parsePacket();
  if (packetSize)
    onReceive(packetSize);
  if (millis() - lastBlink >= BLINK_INTERVAL)
  {
    lastBlink += BLINK_INTERVAL;
    blinkState = !blinkState;
    digitalWrite(LED_BUILTIN, blinkState);
  }
  if (millis() - lastPrint >= PRINT_INTERVAL)
  {
    lastPrint += PRINT_INTERVAL;
    printStations();
  }
  if (!sending && millis() - lastSend > send_delay && !outgoingMessages.empty())
    sendMessage();
}
