// Shaders.h
#pragma once

static const char* vertexShaderSrc = R"(
#version 300 es
in vec4 a_Position;
out vec2 v_TexCoord;
void main() {
    gl_Position = a_Position;
    v_TexCoord = vec2(a_Position.x * 0.5 + 0.5, a_Position.y * 0.5 + 0.5);
})";

static const char* backgroundFragmentSrc = R"(
#version 300 es
#extension GL_OES_EGL_image_external : require
precision mediump float;
uniform samplerExternalOES u_Texture;
in vec2 v_TexCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(u_Texture, v_TexCoord);
})";

static const char* effectFragmentSrc = R"(
#version 300 es
precision mediump float;
uniform sampler2D u_Texture;
uniform vec2 u_Fingertips[5];
uniform int u_NumFingers;
uniform vec2 u_ScreenSize;
in vec2 v_TexCoord;
out vec4 fragColor;

bool pointInPolygon(vec2 point, vec2 polygon[5], int count) {
    bool inside = false;
    for (int i = 0, j = count-1; i < count; j = i++) {
        vec2 pi = polygon[i] * u_ScreenSize;
        vec2 pj = polygon[j] * u_ScreenSize;
        if ((pi.y > point.y) != (pj.y > point.y)) {
            float intersect = (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x;
            if (point.x < intersect) inside = !inside;
        }
    }
    return inside;
}

void main() {
    vec2 pixelCoord = v_TexCoord * u_ScreenSize; // koordinat layar
    vec2 tips[5] = u_Fingertips;
    if (pointInPolygon(pixelCoord, tips, u_NumFingers)) {
        // Efek distorsi sederhana: chromatic aberration
        vec2 uv = v_TexCoord;
        float r = texture(u_Texture, uv + vec2(0.01, 0.0)).r;
        float g = texture(u_Texture, uv).g;
        float b = texture(u_Texture, uv - vec2(0.01, 0.0)).b;
        fragColor = vec4(r, g, b, 1.0);
    } else {
        fragColor = texture(u_Texture, v_TexCoord);
    }
})";
