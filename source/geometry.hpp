#pragma once
#include <vector>
#include <cstdint>

struct Vertex {
    float pos[3];
    float color[3];
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

Mesh createDodecahedron();

// Функция для создания простого куба (понадобится для 6-го доп. задания)
Mesh createCube();