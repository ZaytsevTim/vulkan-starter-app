#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include "geometry.hpp"

namespace graphics {

// Структура для передачи матриц в шейдер (должна совпадать с UBO в GLSL)
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

// Структура для передачи цвета в шейдер
struct ColorBufferObject {
    alignas(16) glm::vec4 uiColor;
};

// Создаёт буфер (например, для вершин или индексов)
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                  VkBuffer& buffer, VmaAllocation& allocation);

// Копирует данные из CPU-памяти в GPU-буфер
void uploadBuffer(VkBuffer dstBuffer, VmaAllocation dstAllocation, const void* srcData, VkDeviceSize size);

// Создаёт вершинный и индексный буферы для меша
void createMeshBuffers(const Mesh& mesh, VkBuffer& vertexBuffer, VmaAllocation& vertexAllocation,
                       VkBuffer& indexBuffer, VmaAllocation& indexAllocation);

// Создание описания дескрипторов (какие буферы будут доступны шейдерам)
void createDescriptorSetLayout(VkDescriptorSetLayout& layout);

// Создание графического пайплайна
void createGraphicsPipeline(VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
                            VkDescriptorSetLayout descriptorSetLayout,
                            VkRenderPass renderPass, VkExtent2D extent);

// Создание пула дескрипторов и самих дескриптор-сетов
void createDescriptorPool(VkDescriptorPool& pool, uint32_t count);
void createDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout layout,
                          VkDescriptorSet* sets, uint32_t count,
                          VkBuffer uniformBuffer, VkBuffer colorBuffer);

// Создание uniform-буферов (матрицы и цвет)
void createUniformBuffers(VkBuffer& uniformBuffer, VmaAllocation& uniformAlloc,
                          VkBuffer& colorBuffer, VmaAllocation& colorAlloc);

} // namespace graphics

