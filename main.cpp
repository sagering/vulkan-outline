// clang-format off
#include <vulkan\vulkan_core.h>
#include <GLFW\glfw3.h>
// clang-format on

#include "window.h"
#include "renderer.h"

#include "input.h"
#include "camera.h"
#include "clock.h"

struct Rippler
{
  Rippler(Clock* clock)
  {
    srand(static_cast<unsigned>(clock->GetNow()));
    rippleEvery = randomFloat(0.1f, 2.f);
  }

  void UpdateRipples(Renderer::Ubo* ubo, Clock* clock)
  {
    const int num_ripples = 10;

    float tick = clock->GetTick();

    timePassed += tick;

    for (int i = 0; i < num_ripples; ++i) {
      ubo->ripple[i].t += tick;
    }

    if (timePassed > rippleEvery) {
      ubo->ripple[nextRipple] = { randomOrigin(), 0.f, .3f, .2f, 1.f, 10.f };
      nextRipple = (nextRipple + 1) % num_ripples;
      timePassed = 0.f;
      rippleEvery = randomFloat(0.1f, 2.f);
    }
  }

private:
  float rippleEvery = 0.f;
  float timePassed = 0.f;
  uint32_t nextRipple = 0;

  float randomFloat(float lo, float hi)
  {
    return lo + static_cast<float>(rand()) /
                  (static_cast<float>(RAND_MAX / (hi - lo)));
  }

  glm::vec3 randomOrigin()
  {
    return { randomFloat(-1.f, 1.f), randomFloat(-1.f, 1.f), 0.f };
  }
};

int
main()
{
  Window window(1280, 920, "Ripples");
  Renderer renderer(&window);
  Camera cam(70.f, 1280.f / 920.f, 0.1f, 1000.f);
  cam.SetPosition({ 0.0f, 0.0f, 5.0f });
  Input input = {};
  Clock clock = {};

  clock.Update();
  Rippler rippler(&clock);

  Renderer::Ubo ubo = {};

  while (true) {
    window.Update();
    input.Update(&window);
    renderer.Update();
    clock.Update();
    cam.Update(&input, &clock);

    rippler.UpdateRipples(&ubo, &clock);
    ubo.vp = cam.GetProjView();

    renderer.drawFrame(ubo);
  }

  return 0;
}
