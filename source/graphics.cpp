#include "graphics.hpp"
#include "graphics_internal.hpp"
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace graphics {

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                  VkBuffer& buffer, VmaAllocation& allocation) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    // Добавляем флаг, чтобы память была доступна для записи с CPU
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    if (vmaCreateBuffer(graphics::internal::context.allocator, &bufferInfo, &allocInfo,
                        &buffer, &allocation, nullptr) != VK_SUCCESS) {
        std::cerr << "Failed to create buffer!\n";
    }
}

void uploadBuffer(VkBuffer dstBuffer, VmaAllocation dstAllocation, const void* srcData, VkDeviceSize size) {
    void* data;
    vmaMapMemory(graphics::internal::context.allocator, dstAllocation, &data);
    std::memcpy(data, srcData, static_cast<size_t>(size));
    vmaUnmapMemory(graphics::internal::context.allocator, dstAllocation);
    // Если память не HOST_COHERENT, нужно сбросить кэш
    vmaFlushAllocation(graphics::internal::context.allocator, dstAllocation, 0, VK_WHOLE_SIZE);
}

void createMeshBuffers(const Mesh& mesh, VkBuffer& vertexBuffer, VmaAllocation& vertexAllocation,
                       VkBuffer& indexBuffer, VmaAllocation& indexAllocation) {
    VkDeviceSize vertexSize = sizeof(Vertex) * mesh.vertices.size();
    VkDeviceSize indexSize = sizeof(uint16_t) * mesh.indices.size();

    // Создаём буферы (используем VMA_MEMORY_USAGE_AUTO, чтобы VMA сам выбрал тип памяти)
    createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                 vertexBuffer, vertexAllocation);
    createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
                 indexBuffer, indexAllocation);

    // Загружаем данные
    uploadBuffer(vertexBuffer, vertexAllocation, mesh.vertices.data(), vertexSize);
    uploadBuffer(indexBuffer, indexAllocation, mesh.indices.data(), indexSize);
}

// --- Вспомогательная функция для чтения файла шейдера ---
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return {};
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    std::cout << "Successfully opened: " << filename << ", size: " << fileSize << std::endl;
    return buffer;
}

static VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(graphics::internal::context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module!\n";
    }
    std::cout << "Shader module created, size: " << code.size() << " bytes\n";
    return shaderModule;
}

void createDescriptorSetLayout(VkDescriptorSetLayout& layout) {
    VkDescriptorSetLayoutBinding bindings[2]{};

    // Binding 0: Uniform Buffer (матрицы)
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // Binding 1: Uniform Buffer (цвет)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(graphics::internal::context.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout!\n";
    }
}

void createGraphicsPipeline(VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
                            VkDescriptorSetLayout descriptorSetLayout,
                            VkRenderPass renderPass, VkExtent2D extent) {
    auto vertCode = readFile("shaders/shader.vert.spv");
    auto fragCode = readFile("shaders/shader.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2]{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // отсечение выключено: depth-тест сам скроет задние грани
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // ВАЖНО: динамический viewport и scissor
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(graphics::internal::context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout!\n";
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(graphics::internal::context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline!\n";
    }

    vkDestroyShaderModule(graphics::internal::context.device, fragModule, nullptr);
    vkDestroyShaderModule(graphics::internal::context.device, vertModule, nullptr);
}

void createDescriptorPool(VkDescriptorPool& pool, uint32_t count) {
    VkDescriptorPoolSize poolSizes[2]{};
    // Для Uniform Buffer (матрицы)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = count;
    // Для Uniform Buffer (цвет)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = count;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = count;

    if (vkCreateDescriptorPool(graphics::internal::context.device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool!\n";
    }
}

void createDescriptorSets(VkDescriptorPool pool, VkDescriptorSetLayout layout,
                          VkDescriptorSet* sets, uint32_t count,
                          VkBuffer uniformBuffer, VkBuffer colorBuffer) {
    std::vector<VkDescriptorSetLayout> layouts(count, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(graphics::internal::context.device, &allocInfo, sets) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor sets!\n";
    }

    for (uint32_t i = 0; i < count; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorBufferInfo colorInfo{};
        colorInfo.buffer = colorBuffer;
        colorInfo.offset = 0;
        colorInfo.range = sizeof(ColorBufferObject);

        VkWriteDescriptorSet descriptorWrites[2]{};
        // Запись для матриц
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // Запись для цвета
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &colorInfo;

        vkUpdateDescriptorSets(graphics::internal::context.device, 2, descriptorWrites, 0, nullptr);
    }
}

void createUniformBuffers(VkBuffer& uniformBuffer, VmaAllocation& uniformAlloc,
                          VkBuffer& colorBuffer, VmaAllocation& colorAlloc) {
    createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VMA_MEMORY_USAGE_AUTO, uniformBuffer, uniformAlloc);
    createBuffer(sizeof(ColorBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VMA_MEMORY_USAGE_AUTO, colorBuffer, colorAlloc);
}

} // namespace graphics

