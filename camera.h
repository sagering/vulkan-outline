#pragma once

#include "clock.h"
#include "input.h"
#include <glm/glm.hpp>

struct Camera
{
  Camera(float fovy, float aspect, float near, float far)
    : fovy(fovy)
    , aspect(aspect)
    , near(near)
    , far(far)
    , pitch(0.f)
    , yaw(0.f)
  {
    UpdateProj();
    UpdateProjView();
  };

  Camera()
    : fovy(0.f)
    , aspect(0.f)
    , near(0.f)
    , far(0.f)
    , pitch(0.f)
    , yaw(0.f)
  {
    UpdateProj();
    UpdateProjView();
  };

  void SetPosition(glm::vec3 pos);
  void Update(Input* input, Clock* clock);

  glm::mat4 GetProjView() { return projView; }

private:
  // par
  float fovy;
  float aspect;
  float near;
  float far;

  // state
  float pitch;
  float yaw;
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 projView;
  glm::mat4 rotation;
  glm::vec3 t0; // translation in world coordinates
  glm::vec3 t1; // translation in camera coordinates

  void UpdateProj();
  void UpdateView();
  void UpdateProjView();
};
