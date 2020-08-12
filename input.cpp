#include "input.h"

void
Input::Update(Window* window)
{
  cursor.dx = 0.f;
  cursor.dy = 0.f;

  if (window->cursorInput.updated) {
    cursor.dx = static_cast<float>(window->cursorInput.dx);
    cursor.dy = static_cast<float>(window->cursorInput.dy);
  }

  memcpy(&keyboard.key, &window->keyboardState.key, sizeof(keyboard.key));
}
