#include "camera.h"
#include <glm\gtx\transform.hpp>

void
Camera::SetPosition(glm::vec3 pos)
{
  t0 = pos;
  UpdateView();
  UpdateProjView();
}

void
Camera::Update(Input* input, Clock* clock)
{
  float tick = clock->GetTick();

  const float speed = 1.0f;
  bool viewChanged = false;
  bool projChanged = false;

  if (input->cursor.dx != 0.f || input->cursor.dy != 0.f) {
    yaw += 0.01f * input->cursor.dx;
    pitch -= 0.01f * input->cursor.dy;

    rotation = glm::rotate(pitch, glm::vec3(1.f, 0.f, 0.f)) *
               glm::rotate(yaw, glm::vec3(0.f, 1.f, 0.f));

    viewChanged = true;
  }

  const auto local_z =
    glm::vec3(rotation[0][2], rotation[1][2], rotation[2][2]);
  const auto local_x =
    glm::vec3(rotation[0][0], rotation[1][0], rotation[2][0]);

  if (input->keyboard.key[GLFW_KEY_W] == GLFW_PRESS) {
    t0 -= speed * local_z * tick;
    viewChanged = true;
  }

  if (input->keyboard.key[GLFW_KEY_S] == GLFW_PRESS) {
    t0 += speed * local_z * tick;
    viewChanged = true;
  }

  if (input->keyboard.key[GLFW_KEY_A] == GLFW_PRESS) {
    t0 -= speed * local_x * tick;
    viewChanged = true;
  }

  if (input->keyboard.key[GLFW_KEY_D] == GLFW_PRESS) {
    t0 += speed * local_x * tick;
    viewChanged = true;
  }

  if (input->keyboard.key[GLFW_KEY_K] == GLFW_PRESS) {
    fovy = glm::clamp(fovy + 1, 30.f, 160.f);
    projChanged = true;
  }

  if (input->keyboard.key[GLFW_KEY_J] == GLFW_PRESS) {
    fovy = glm::clamp(fovy - 1, 30.f, 160.f);
    projChanged = true;
  }

  if (viewChanged) {
    UpdateView();
  }

  if (projChanged) {
    UpdateProj();
  }

  if (viewChanged || projChanged) {
    UpdateProjView();
  }
};

void
Camera::UpdateProjView()
{
  projView = proj * view;
}

void
Camera::UpdateView()
{
  t1 = glm::vec3(rotation * glm::vec4(t0.x, t0.y, t0.z, 0));
  view = rotation;
  view[3][0] = -t1.x;
  view[3][1] = -t1.y;
  view[3][2] = -t1.z;
  view[3][3] = 1.f;
  view[1][1] *= -1.f;
}

void
Camera::UpdateProj()
{
  proj = glm::perspective(fovy, aspect, near, far);
}
