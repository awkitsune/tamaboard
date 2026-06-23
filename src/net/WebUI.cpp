#include "WebUI.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "../Input.h"
#include "../app/Controller.h"
#include "StateBroadcaster.h"

static Controller *_ctrl;
static StateBroadcaster *_bcast;

// ---- WS message parsing (tiny — avoid pulling in ArduinoJson for so little)
// ----
//
// Wire format (client → server), all JSON text frames:
//   {"t":"action", "state":"EAT|PLAY|SLEEP|IDLE|KEYBOARD"}
//   {"t":"slot",   "i":0, "action":"select|pair|forget"}
//   {"t":"clock",  "local_seconds":12345}
//   {"t":"key",    "code":4, "down":true}
//   {"t":"move",   "dx":5, "dy":-3}
//   {"t":"click",  "btn":1}
//
// Server → client messages are built by StateBroadcaster; see its source.

static State parseStateName(const String &s) {
  if (s == "EAT")
    return State::EAT;
  if (s == "PLAY")
    return State::PLAY;
  if (s == "SLEEP")
    return State::SLEEP;
  if (s == "KEYBOARD")
    return State::KEYBOARD;
  return State::IDLE;
}

static Controller::SlotAction parseSlotAction(const String &s) {
  if (s == "pair")
    return Controller::SlotAction::PAIR;
  if (s == "forget")
    return Controller::SlotAction::FORGET;
  return Controller::SlotAction::SELECT;
}

// Pull a quoted string value out of a JSON-ish blob: substring after "key":" up
// to the next ". Good enough for our 1-level messages, no nesting/escapes.
static String fieldString(const String &s, const char *key) {
  String needle = String("\"") + key + "\":\"";
  int i = s.indexOf(needle);
  if (i < 0)
    return String();
  int start = i + needle.length();
  int end = s.indexOf('"', start);
  if (end < 0)
    return String();
  return s.substring(start, end);
}
static int fieldInt(const String &s, const char *key) {
  String needle = String("\"") + key + "\":";
  int i = s.indexOf(needle);
  if (i < 0)
    return 0;
  return s.substring(i + needle.length()).toInt();
}
static bool fieldBool(const String &s, const char *key) {
  String needle = String("\"") + key + "\":true";
  return s.indexOf(needle) >= 0;
}

static void onMessage(const char *data, size_t len) {
  String s(data, len);
  String t = fieldString(s, "t");

  if (t == "action") {
    _ctrl->transitionState(parseStateName(fieldString(s, "state")));
  } else if (t == "slot") {
    _ctrl->slotAction((uint8_t)fieldInt(s, "i"),
                      parseSlotAction(fieldString(s, "action")));
  } else if (t == "clock") {
    _ctrl->syncClock((uint32_t)fieldInt(s, "local_seconds"));
  } else if (t == "key") {
    InputEvent e{InputType::KEY};
    e.code = (uint8_t)fieldInt(s, "code");
    e.down = fieldBool(s, "down");
    _ctrl->hidInput(e);
  } else if (t == "move") {
    InputEvent e{InputType::MOVE};
    e.dx = (int8_t)fieldInt(s, "dx");
    e.dy = (int8_t)fieldInt(s, "dy");
    _ctrl->hidInput(e);
  } else if (t == "click") {
    InputEvent e{InputType::CLICK};
    e.button = (uint8_t)fieldInt(s, "btn");
    _ctrl->hidInput(e);
  }
}

void WebUI::begin(Controller &controller, StateBroadcaster &broadcaster) {
  _ctrl = &controller;
  _bcast = &broadcaster;

  _ws.onEvent([](AsyncWebSocket *, AsyncWebSocketClient *client,
                 AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      _bcast->sendInitial(*client);
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->opcode == WS_TEXT)
        onMessage((char *)data, len);
    }
  });

  _server.addHandler(&_ws);
  if (_debugFn) {
    _server.on("/debug", HTTP_GET, [this](AsyncWebServerRequest *req) {
      req->send(200, "text/html", _debugFn());
    });
  }
  _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  _server.begin();
  Serial.println("[WebUI] HTTP+WS up on :80");
}
