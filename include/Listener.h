#pragma once

#include <cstdint>

using ListenerID = uint32_t;

struct Listener {
  ListenerID id = 0;
  float posX = 0, posY = 0, posZ = 0;
  float velX = 0, velY = 0, velZ = 0;
  float forwardX = 0, forwardY = 0, forwardZ = -1;
  float upX = 0, upY = 1, upZ = 0;
  bool active = true;
};
