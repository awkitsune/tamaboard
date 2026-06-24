#pragma once
#include <ESPAsyncWebServer.h>

#include <functional>

class Controller;
class StateBroadcaster;

class WebUI {
public:
  void begin(Controller &controller, StateBroadcaster &broadcaster);

  // Call before begin(). If set, registers GET /debug returning fn().
  void setDebugProvider(std::function<String()> fn) {
    _debugFn = std::move(fn);
  }

  AsyncWebSocket &websocket() { return _ws; }

private:
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};
  std::function<String()> _debugFn;
};
