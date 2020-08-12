#include "vk_utils.h"
#include <string>

std::string
loadFile(const char* filename)
{
  std::string buffer;

  FILE* file = 0;
  fopen_s(&file, filename, "rb");

  if (file) {
    fseek(file, 0, SEEK_END);
    size_t bytes = ftell(file);

    buffer.resize(bytes);

    fseek(file, 0, SEEK_SET);
    fread(&(*buffer.begin()), 1, bytes, file);
    fclose(file);
    file = 0;
    return buffer;
  }

  return buffer;
}

VkShaderModule
vkuLoadShaderModule(VkDevice device, char* filename)
{
  std::string buffer = loadFile(filename);

  VkShaderModule shader_module = vkuCreateShaderModule(
    device, buffer.size(), reinterpret_cast<const uint32_t*>(buffer.data()));

  return shader_module;
}
