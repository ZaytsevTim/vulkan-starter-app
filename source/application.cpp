#include "application.hpp"
#include "graphics.hpp"
#include "geometry.hpp"
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

namespace application {

VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
VkPipeline pipeline = VK_NULL_HANDLE;
VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
VkDescriptorSet descriptorSets[2];

VkBuffer vertexBuffer1, indexBuffer1;
VmaAllocation vertexAlloc1, indexAlloc1;
uint32_t indexCount1 = 0;

VkBuffer vertexBuffer2, indexBuffer2;
VmaAllocation vertexAlloc2, indexAlloc2;
uint32_t indexCount2 = 0;

VkBuffer uniformBuffer1, colorBuffer1;
VmaAllocation uniformAlloc1, colorAlloc1;
VkBuffer uniformBuffer2, colorBuffer2;
VmaAllocation uniformAlloc2, colorAlloc2;

bool isPerspective = true;
glm::vec3 position(0.0f, 0.0f, 0.0f);
glm::vec3 rotation(0.0f, 0.0f, 0.0f);
glm::vec3 scale(1.0f, 1.0f, 1.0f);
glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
bool isAnimating = false;
float animationSpeed = 1.0f;
float animTime = 0.0f;
float trajectoryRadius = 1.0f;
bool moveOnTrajectory = true;
bool spinWhileAnimating = true;

glm::vec3 position2(1.5f, 0.0f, 0.0f);
glm::vec4 color2(1.0f, 0.5f, 0.0f, 1.0f);

bool initialize() {
    auto& ctx = graphics::internal::context;
    std::cout << "--- Initializing Application ---" << std::endl;

    graphics::createDescriptorSetLayout(descriptorSetLayout);
    graphics::createGraphicsPipeline(pipelineLayout, pipeline, descriptorSetLayout,
                                     ctx.render_pass, ctx.swapchain_extent);

    Mesh dodecahedron = createDodecahedron();
    Mesh cube = createCube();
    indexCount1 = static_cast<uint32_t>(dodecahedron.indices.size());
    indexCount2 = static_cast<uint32_t>(cube.indices.size());

    graphics::createMeshBuffers(dodecahedron, vertexBuffer1, vertexAlloc1, indexBuffer1, indexAlloc1);
    graphics::createMeshBuffers(cube, vertexBuffer2, vertexAlloc2, indexBuffer2, indexAlloc2);

    graphics::createUniformBuffers(uniformBuffer1, uniformAlloc1, colorBuffer1, colorAlloc1);
    graphics::createUniformBuffers(uniformBuffer2, uniformAlloc2, colorBuffer2, colorAlloc2);

    graphics::createDescriptorPool(descriptorPool, 2);
    
    VkDescriptorSet sets[2];
    graphics::createDescriptorSets(descriptorPool, descriptorSetLayout, sets, 2, uniformBuffer1, colorBuffer1);
    descriptorSets[0] = sets[0];
    descriptorSets[1] = sets[1];

    VkDescriptorBufferInfo bufferInfo2{};
    bufferInfo2.buffer = uniformBuffer2;
    bufferInfo2.offset = 0;
    bufferInfo2.range = sizeof(graphics::UniformBufferObject);
    VkDescriptorBufferInfo colorInfo2{};
    colorInfo2.buffer = colorBuffer2;
    colorInfo2.offset = 0;
    colorInfo2.range = sizeof(graphics::ColorBufferObject);

    VkWriteDescriptorSet writes2[2]{};
    writes2[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes2[0].dstSet = descriptorSets[1];
    writes2[0].dstBinding = 0;
    writes2[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes2[0].descriptorCount = 1;
    writes2[0].pBufferInfo = &bufferInfo2;
    writes2[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes2[1].dstSet = descriptorSets[1];
    writes2[1].dstBinding = 1;
    writes2[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes2[1].descriptorCount = 1;
    writes2[1].pBufferInfo = &colorInfo2;

    vkUpdateDescriptorSets(ctx.device, 2, writes2, 0, nullptr);
    
    std::cout << "--- Initialization Complete ---" << std::endl;
    return true;
}

void shutdown() {
    auto& ctx = graphics::internal::context;
    vkQueueWaitIdle(ctx.graphics_queue);

    vmaDestroyBuffer(ctx.allocator, vertexBuffer1, vertexAlloc1);
    vmaDestroyBuffer(ctx.allocator, indexBuffer1, indexAlloc1);
    vmaDestroyBuffer(ctx.allocator, vertexBuffer2, vertexAlloc2);
    vmaDestroyBuffer(ctx.allocator, indexBuffer2, indexAlloc2);
    vmaDestroyBuffer(ctx.allocator, uniformBuffer1, uniformAlloc1);
    vmaDestroyBuffer(ctx.allocator, colorBuffer1, colorAlloc1);
    vmaDestroyBuffer(ctx.allocator, uniformBuffer2, uniformAlloc2);
    vmaDestroyBuffer(ctx.allocator, colorBuffer2, colorAlloc2);

    vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, descriptorSetLayout, nullptr);
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);
}

void update(double time) {
    (void)time;
    if (isAnimating) {
        animTime += ImGui::GetIO().DeltaTime * animationSpeed;
    }

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 420), ImGuiCond_FirstUseEver);

    ImGui::Begin("Lab 1 - Dodecahedron");
    ImGui::Text("Variant 10: Dodecahedron");
    ImGui::Separator();
    ImGui::Checkbox("Perspective Projection", &isPerspective);
    ImGui::SliderFloat3("Position", glm::value_ptr(position), -5.0f, 5.0f);
    ImGui::SliderFloat3("Rotation", glm::value_ptr(rotation), -3.14f, 3.14f);
    ImGui::SliderFloat3("Scale", glm::value_ptr(scale), 0.1f, 3.0f);
    ImGui::ColorEdit4("Color", glm::value_ptr(color));
    ImGui::Separator();
    ImGui::Checkbox("Play Animation", &isAnimating);
    ImGui::Checkbox("Move along trajectory", &moveOnTrajectory);
    ImGui::Checkbox("Spin", &spinWhileAnimating);
    ImGui::SliderFloat("Animation Speed", &animationSpeed, 0.0f, 5.0f);
    ImGui::SliderFloat("Trajectory Radius", &trajectoryRadius, 0.0f, 5.0f);
    ImGui::Separator();
    ImGui::Text("Second Object (Cube)");
    ImGui::ColorEdit4("Cube Color", glm::value_ptr(color2));
    ImGui::End();
}

void render(const graphics::internal::FrameData& fd) {
    auto& ctx = graphics::internal::context;

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(fd.command_buffer, &begin_info);

    VkClearValue clearValues[2]{};
    clearValues[0].color = {{0.1f, 0.1f, 0.15f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx.render_pass;
    renderPassInfo.framebuffer = fd.framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = ctx.swapchain_extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(fd.command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(ctx.swapchain_extent.width);
    viewport.height = static_cast<float>(ctx.swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ctx.swapchain_extent;
    vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

    // Камера смотрит строго на центр с расстояния 3
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj;
    if (isPerspective) {
        proj = glm::perspective(glm::radians(45.0f), 
            ctx.swapchain_extent.width / (float)ctx.swapchain_extent.height, 0.1f, 100.0f);
    } else {
        float aspect = ctx.swapchain_extent.width / (float)ctx.swapchain_extent.height;
        proj = glm::ortho(-2.0f * aspect, 2.0f * aspect, -2.0f, 2.0f, 0.1f, 100.0f);
    }
    proj[1][1] *= -1.0f; // в Vulkan ось Y NDC направлена вниз

    // --- Объект 1: Додекаэдр (позиция/поворот/масштаб из UI + круговая траектория) ---
    glm::vec3 pos1 = position;
    glm::vec3 rot1 = rotation;
    if (isAnimating) {
        if (moveOnTrajectory) {
            // Сложная траектория: фигура Лиссажу в 3D
            pos1.x += std::cos(animTime) * trajectoryRadius;
            pos1.y += std::sin(2.0f * animTime) * trajectoryRadius * 0.5f;
            pos1.z += std::sin(animTime) * trajectoryRadius * 0.3f;
        }
        if (spinWhileAnimating) {
            // Вращение вокруг собственных осей
            rot1 += glm::vec3(0.7f, 1.0f, 0.4f) * animTime;
        }
    }
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), pos1);
    model1 = glm::rotate(model1, rot1.x, glm::vec3(1.0f, 0.0f, 0.0f));
    model1 = glm::rotate(model1, rot1.y, glm::vec3(0.0f, 1.0f, 0.0f));
    model1 = glm::rotate(model1, rot1.z, glm::vec3(0.0f, 0.0f, 1.0f));
    model1 = glm::scale(model1, scale);

    graphics::UniformBufferObject ubo1{};
    ubo1.model = model1;
    ubo1.view = view;
    ubo1.proj = proj;

    graphics::ColorBufferObject cbo1{};
    cbo1.uiColor = color;

    graphics::uploadBuffer(uniformBuffer1, uniformAlloc1, &ubo1, sizeof(ubo1));
    graphics::uploadBuffer(colorBuffer1, colorAlloc1, &cbo1, sizeof(cbo1));

    VkBuffer vertexBuffers1[] = {vertexBuffer1};
    VkDeviceSize offsets1[] = {0};
    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, vertexBuffers1, offsets1);
    vkCmdBindIndexBuffer(fd.command_buffer, indexBuffer1, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[0], 0, nullptr);
    vkCmdDrawIndexed(fd.command_buffer, indexCount1, 1, 0, 0, 0);

    // --- Объект 2: Куб (справа) ---
    glm::mat4 model2 = glm::mat4(1.0f);
    model2 = glm::translate(model2, position2);
    model2 = glm::scale(model2, glm::vec3(0.5f));

    graphics::UniformBufferObject ubo2{};
    ubo2.model = model2;
    ubo2.view = view;
    ubo2.proj = proj;

    graphics::ColorBufferObject cbo2{};
    cbo2.uiColor = color2;

    graphics::uploadBuffer(uniformBuffer2, uniformAlloc2, &ubo2, sizeof(ubo2));
    graphics::uploadBuffer(colorBuffer2, colorAlloc2, &cbo2, sizeof(cbo2));

    VkBuffer vertexBuffers2[] = {vertexBuffer2};
    VkDeviceSize offsets2[] = {0};
    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, vertexBuffers2, offsets2);
    vkCmdBindIndexBuffer(fd.command_buffer, indexBuffer2, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[1], 0, nullptr);
    vkCmdDrawIndexed(fd.command_buffer, indexCount2, 1, 0, 0, 0);

    vkCmdEndRenderPass(fd.command_buffer);
    vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application