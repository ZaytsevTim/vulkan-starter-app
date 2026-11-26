#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragViewPos;

layout(binding = 1) uniform ColorBufferObject {
    vec4 uiColor;
} cbo;

layout(location = 0) out vec4 outColor;

void main() {
    // Плоская нормаль грани из производных позиции (вершинный формат менять не нужно)
    vec3 n = normalize(cross(dFdx(fragViewPos), dFdy(fragViewPos)));
    if (dot(n, -fragViewPos) < 0.0) n = -n; // нормаль смотрит на камеру

    vec3 lightDir = normalize(vec3(0.5, 0.8, 1.0)); // свет в системе камеры
    float diffuse = max(dot(n, lightDir), 0.0);
    float light = 0.35 + 0.65 * diffuse;            // ambient + diffuse

    vec3 base = fragColor * cbo.uiColor.rgb;       // цвет вершин * цвет из UI (доп. задание 5)
    outColor = vec4(base * light, cbo.uiColor.a);
}