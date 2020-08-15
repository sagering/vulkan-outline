// clang-format off
#include <vulkan\vulkan_core.h>
#include <GLFW\glfw3.h>
// clang-format on

#include <glm\gtx\transform.hpp>

#include "window.h"
#include "renderer.h"

#include "input.h"
#include "camera.h"
#include "clock.h"

int
main()
{
  {
    Window window(1280, 920, "Outline");
    Renderer renderer(&window);
    Camera cam(70.f, 1280.f / 920.f, 0.1f, 1000.f);
    cam.SetPosition({ 0.0f, -1.0f, 7.0f });
    Input input = {};
    Clock clock = {};

    clock.Update();

    Renderer::Ubo ubo = {};

    while (window.keyboardState.key[GLFW_KEY_ESCAPE] != 1) {
      window.Update();
      input.Update(&window);
      renderer.Update();
      clock.Update();
      cam.Update(&input, &clock);

      ubo.vp = cam.GetProjView();
      ubo.m =
        glm::rotate(ubo.m, clock.GetTick() * 0.5f, glm::vec3(0.f, 1.f, 0.f));
      ubo.res = glm::ivec2(window.windowSize.width, window.windowSize.height);

      renderer.drawFrame(ubo);
    }
  }

  return 0;
}
