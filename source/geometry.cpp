#include "geometry.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {

struct V3 { float x, y, z; };

float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
V3 normalize(V3 a) { float l = std::sqrt(dot(a, a)); return { a.x / l, a.y / l, a.z / l }; }

Vertex makeVertex(float x, float y, float z) {
    Vertex v;
    v.pos[0] = x; v.pos[1] = y; v.pos[2] = z;
    // Процедурный цвет: координаты из [-1, 1] в [0, 1]
    v.color[0] = (x + 1.0f) * 0.5f;
    v.color[1] = (y + 1.0f) * 0.5f;
    v.color[2] = (z + 1.0f) * 0.5f;
    return v;
}

} // namespace

Mesh createDodecahedron() {
    Mesh mesh;
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
    const float ip = 1.0f / phi;

    // 20 вершин: (±1,±1,±1), (0,±1/φ,±φ), (±1/φ,±φ,0), (±φ,0,±1/φ)
    std::vector<V3> pts;
    for (float x : {-1.0f, 1.0f})
        for (float y : {-1.0f, 1.0f})
            for (float z : {-1.0f, 1.0f})
                pts.push_back({x, y, z});
    for (float a : {-1.0f, 1.0f})
        for (float b : {-1.0f, 1.0f}) {
            pts.push_back({0.0f, a * ip, b * phi});
            pts.push_back({a * ip, b * phi, 0.0f});
            pts.push_back({a * phi, 0.0f, b * ip});
        }

    // Нормализуем: все вершины лежат на сфере радиуса sqrt(3)
    const float R = std::sqrt(3.0f);
    for (auto& p : pts) {
        p = { p.x / R, p.y / R, p.z / R };
        mesh.vertices.push_back(makeVertex(p.x, p.y, p.z));
    }

    // Нормали 12 граней додекаэдра: (0,±φ,±1), (±1,0,±φ), (±φ,±1,0)
    std::vector<V3> normals;
    for (float a : {-1.0f, 1.0f})
        for (float b : {-1.0f, 1.0f}) {
            normals.push_back({0.0f, a * phi, b});
            normals.push_back({a, 0.0f, b * phi});
            normals.push_back({a * phi, b, 0.0f});
        }

    // Для каждой грани берём 5 вершин, ближайших к её нормали,
    // сортируем по углу вокруг нормали (против часовой, если смотреть снаружи)
    // и режем веером на 3 треугольника.
    for (V3 n : normals) {
        n = normalize(n);

        float best = -2.0f;
        for (auto& p : pts) best = std::max(best, dot(p, n));

        std::vector<uint16_t> face;
        for (size_t i = 0; i < pts.size(); ++i)
            if (dot(pts[i], n) > best - 1e-3f) face.push_back(static_cast<uint16_t>(i));

        V3 axis = (std::fabs(n.x) < 0.9f) ? V3{1, 0, 0} : V3{0, 1, 0};
        V3 u = normalize(cross(n, axis));
        V3 w = cross(n, u);

        std::vector<std::pair<float, uint16_t>> sorted;
        for (uint16_t idx : face)
            sorted.push_back({ std::atan2(dot(pts[idx], w), dot(pts[idx], u)), idx });
        std::sort(sorted.begin(), sorted.end());

        for (size_t i = 1; i + 1 < sorted.size(); ++i) {
            mesh.indices.push_back(sorted[0].second);
            mesh.indices.push_back(sorted[i].second);
            mesh.indices.push_back(sorted[i + 1].second);
        }
    }

    return mesh; // 12 граней * 3 треугольника * 3 = 108 индексов
}

Mesh createCube() {
    Mesh mesh;
    float verts[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };
    for (int i = 0; i < 24; i++)
        mesh.vertices.push_back(makeVertex(verts[i*3+0], verts[i*3+1], verts[i*3+2]));

    uint16_t indices[] = {
        0, 1, 2, 2, 3, 0,      4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,   12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20
    };
    for (uint16_t idx : indices) mesh.indices.push_back(idx);
    return mesh;
}
