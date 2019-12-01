#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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

IPAddress Ip(192, 168, 1, 1); // ip address for website
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct Message
{
  std::string payload;
  byte radioDestination;
  byte idDestination;
  byte radioSender;
  byte idSender;
  byte id;             // message send count / id
  byte frame;          // frame number
  byte packet;         // packet count / number
  byte packetLocation; // packet start, mid, or end
  byte type;
  int hash;
  Message(std::string _payload, byte _radioDestination, byte _idDestination, byte _radioSender, byte _idSender, byte _id, byte _frame, byte _packet, byte _packetLocation, byte _type) : payload{_payload},
                                                                                                                                                                                         radioDestination{_radioDestination},
                                                                                                                                                                                         idDestination{_idDestination},
                                                                                                                                                                                         radioSender(_radioSender),
                                                                                                                                                                                         idSender(_idSender),
                                                                                                                                                                                         id{_id},
                                                                                                                                                                                         frame{_frame},
                                                                                                                                                                                         packet{_packet},
                                                                                                                                                                                         packetLocation{_packetLocation},
                                                                                                                                                                                         type{_type}
  {
    hash = std::hash<std::string>{}(payload + (char)radioDestination + (char)idDestination + (char)radioSender + (char)idSender + (char)id + (char)frame + (char)packet + (char)packetLocation + (char)type);
  }
};

std::unordered_map<byte, AsyncWebSocketClient *> websocketClientsById;
std::unordered_map<AsyncWebSocketClient *, byte> websocketClientsByVal;
std::unordered_map<AsyncWebSocketClient *, std::pair<byte, byte>> sendSocketStatus;
std::queue<Message> outgoingMessages;
std::queue<int *> hashQueue;
std::unordered_set<int> hashHistory;
static bool sending = false;
unsigned long lastSend = 0;
byte msgCount = 0; // count of outgoing messages
std::vector<Message> incomingMessages;
std::unordered_map<AsyncWebSocketClient *, std::vector<byte>> messageHeaders;
std::unordered_set<std::string> connectedOverRadio;

byte startType = 0xF0;
byte endType = 0x0F;
byte midType = 0x00;
byte startEndType = 0xFF;
byte typeMessage = 0x00;
byte typeConnect = 0x01;
byte typeDisconnect = 0x02;
byte typePotentialConnections = 0x03;

static const unsigned long BLINK_INTERVAL = 1000; //ms
static unsigned long lastBlink = 0;
static bool blinkState = false; // false = off

void saveMessages(std::string fullmessage, byte radioDestination, byte idDestination, byte idSender, byte id, byte frame, byte packetLocation, byte type)
{
  // split into multiple packets
  int endCount = ceil((double)fullmessage.length() / maxLoraPacket);
  for (int i = 0; i < endCount; i++)
  {
    byte location = midType;
    if (i == 0 || i == endCount - 1)
      location = packetLocation;
    outgoingMessages.push(Message(fullmessage.substr(i * maxLoraPacket, maxLoraPacket), radioDestination, idDestination, address, idSender, id, frame, i, location, type));
  }
}

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return; // if there's no packet, return
  DBG_OUTPUT_PORT.println("received message");
  // read packet header bytes:
  byte radioRecipient = LoRa.read();
  byte idRecipient = LoRa.read();
  byte radioSender = LoRa.read();
  byte idSender = LoRa.read();
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
  Message message(payload, radioRecipient, idRecipient, radioSender, idSender, id, frame, packet, packetLocation, type);
  if (radioRecipient != address)
  {
    if (debug_mode)
      DBG_OUTPUT_PORT.println("this message is not for me. It's for " + String(radioRecipient));
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
  if (message.packetLocation == endType || message.packetLocation == startEndType)
  {
    std::string fullMessage = "";
    for (Message &message : incomingMessages)
      fullMessage.append(message.payload);
    std::string key;
    if (message.type == typeMessage)
    {
      key = "message";
    }
    else if (message.type == typeConnect)
    {
      key = "connect";
      char sender[] = {radioSender, idSender};
      connectedOverRadio.insert(std::string(sender));
    }
    else if (message.type == typeDisconnect)
    {
      key = "disconnect";
      char sender[] = {radioSender, idSender};
      connectedOverRadio.erase(std::string(sender));
    }
    else if (message.type == typePotentialConnections)
    {
      if (payload.length() > 0)
      {
        // send to connected client
        key = "connectionOptions";
      }
      else
      {
        std::string contents;
        for (std::pair<const byte, AsyncWebSocketClient *> keyValue : websocketClientsById)
          contents += keyValue.first;
        saveMessages(contents, radioSender, idSender, idRecipient, msgCount, 0, startEndType, typePotentialConnections);
        return;
      }
    }
    else
    {
      key = "unknown";
    }
    DynamicJsonDocument messageObj(200);
    messageObj[key.c_str()] = fullMessage.c_str();
    String messageStr;
    serializeJson(messageObj, messageStr);
    websocketClientsById[idRecipient]->text(messageStr.c_str());
    incomingMessages.clear();
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 0, String(fullMessage.c_str()));
    Heltec.display->display();
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
  LoRa.write(message.radioDestination);        // add message radio destination
  LoRa.write(message.idDestination);           // add message id destination
  LoRa.write(message.radioSender);             // add local radio address
  LoRa.write(message.idSender);                // add local sender id
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

void handleError(AsyncWebServerRequest *request, int errorCode, String errorMessage)
{
  if (debug_mode)
    DBG_OUTPUT_PORT.println("error: " + errorMessage);
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
    byte id = (byte)websocketClientsById.size();
    websocketClientsByVal[client] = id;
    websocketClientsById[id] = client;
    DynamicJsonDocument successObj(200);
    successObj["currentid"] = String(id);
    String successStr;
    serializeJson(successObj, successStr);
    client->text(successStr.c_str());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    if (debug_mode && verbose)
      DBG_OUTPUT_PORT.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    // delete from map
    byte sender = websocketClientsByVal[client];
    websocketClientsById.erase(sender);
    websocketClientsByVal.erase(client);
    if (sendSocketStatus.find(client) != sendSocketStatus.end())
    {
      byte *radioDestination = &sendSocketStatus[client].first;
      byte *idDestination = &sendSocketStatus[client].second;
      saveMessages("", *radioDestination, *idDestination, sender, msgCount, 0, startEndType, typeDisconnect);
      sendSocketStatus.erase(client);
    }
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

      byte radioDestination;
      byte idDestination;
      byte thetype;
      if (info->opcode == WS_TEXT)
      {
        radioDestination = data[0];
        idDestination = data[0];
        thetype = data[1];
        for (size_t i = 2; i < info->len; i++)
          msg += (char)data[i];
      }
      else
      {
        if (debug_mode && verbose)
          DBG_OUTPUT_PORT.println("received binary data");
        return;
      }
      byte idSender = websocketClientsByVal[client];
      if (thetype == typeConnect)
      {
        sendSocketStatus[client] = std::pair<byte, byte>(address, idSender);
      }
      saveMessages(msg, radioDestination, idDestination, idSender, msgCount, 0, startEndType, thetype);
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

      byte radioDestination;
      byte idDestination;
      byte theType;
      size_t startIndex;
      if (frameType == startType)
      {
        radioDestination = data[0];
        idDestination = data[1];
        theType = data[2];
        messageHeaders[client].push_back(radioDestination);
        messageHeaders[client].push_back(idDestination);
        messageHeaders[client].push_back(type);
        startIndex = 3;
      }
      else
      {
        if (messageHeaders.find(client) == messageHeaders.end())
        {
          if (debug_mode)
            DBG_OUTPUT_PORT.println("could not find client in map");
          return;
        }
        radioDestination = messageHeaders[client][0];
        idDestination = messageHeaders[client][1];
        theType = messageHeaders[client][2];
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
      saveMessages(msg, radioDestination, idDestination, websocketClientsByVal[client], msgCount, info->index, frameType, theType);
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

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#hello get request");
    request->send(200, "text/plain", "Hello World Get");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#config get request");
    DynamicJsonDocument configObj(200);
    configObj["radiosource"] = String(address);
    String dataStr;
    serializeJson(configObj, dataStr);
    request->send(200, "application/json", dataStr.c_str()); });

  server.on("/alreadyConnected", HTTP_GET, [](AsyncWebServerRequest *request) {
    DBG_OUTPUT_PORT.println("#alreadyConnected get request");
    DynamicJsonDocument sendersObj(200);
    JsonArray clients = sendersObj.createNestedArray("connected");
    for (const std::string & sender : connectedOverRadio) {
      if (debug_mode && verbose)
        DBG_OUTPUT_PORT.println(String(sender.c_str()));
      clients.add(String(sender.c_str()));
    }
    String dataStr;
    serializeJson(sendersObj, dataStr);
    request->send(200, "application/json", dataStr.c_str()); });

  server.on("/potentialConnections", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    DBG_OUTPUT_PORT.println("#potentialConnections put request");
    DynamicJsonDocument dataObj(200);
    DBG_OUTPUT_PORT.println(String((const char *)data));
    DeserializationError error = deserializeJson(dataObj, (const char *)data);
    if (error != error.Ok) {
      DBG_OUTPUT_PORT.println("error deserializing");
      handleError(request, 500, error.c_str());
      return;
    }
    if (!dataObj.containsKey("radio")) {
      handleError(request, 500, "no radio key found");
      return;
    }
    if (!dataObj.containsKey("websocketid")) {
      handleError(request, 500, "no websocket id found");
      return;
    }
    saveMessages("", (byte)dataObj["radio"], (byte)0, (byte)dataObj["websocketid"], msgCount, 0, startEndType, typePotentialConnections);
    DynamicJsonDocument resObj(200);
    resObj["message"] = "getting potential connections";
    String resStr;
    serializeJson(resObj, resStr);
    request->send(200, "application/json", resStr.c_str()); });

  server.on("/message", HTTP_PUT, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    DBG_OUTPUT_PORT.println("#message put request");
    DynamicJsonDocument dataObj(200);
    DeserializationError error = deserializeJson(dataObj, (const char *)data);
    if (error != error.Ok) {
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
    if (!dataObj.containsKey("radioDestination")) {
      handleError(request, 500, "no destination key found");
      return;
    }
    if (!dataObj.containsKey("idDestination")) {
      handleError(request, 500, "no destination key found");
      return;
    }
    byte radioDestination = dataObj["radioDestination"];
    byte idDestination = dataObj["idDestination"];
    // one way message - no return data necessary
    saveMessages(message.c_str(), radioDestination, idDestination, (byte)0, msgCount, 0, startEndType, typeMessage);
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
