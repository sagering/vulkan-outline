#pragma once
#include "window.h"
#include <GLFW\glfw3.h>

struct Input
{
  struct Keyboard
  {
    int key[GLFW_KEY_LAST];
  } keyboard;

  struct Cursor
  {
    float dx;
    float dy;
  } cursor;

  void Update(Window* window);
};
