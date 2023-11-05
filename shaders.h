#pragma once
#include "glm/geometric.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "FastNoise.h"
#include "uniforms.h"
#include "fragment.h"
#include "noise.h"
#include "print.h"

static int frame = 0;

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    glm::vec4 clipSpaceVertex = uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);
    glm::vec3 ndcVertex = glm::vec3(clipSpaceVertex) / clipSpaceVertex.w;
    glm::vec4 screenVertex = uniforms.viewport * glm::vec4(ndcVertex, 1.0f);
    glm::vec3 transformedNormal = glm::mat3(uniforms.model) * vertex.normal;
    transformedNormal = glm::normalize(transformedNormal);
    glm::vec3 transformedWorldPosition = glm::vec3(uniforms.model * glm::vec4(vertex.position, 1.0f));

    // Return the transformed vertex as a vec3
    return Vertex{
            glm::vec3(screenVertex),
            transformedNormal,
            vertex.tex,
            transformedWorldPosition,
            vertex.position
    };
}

Fragment sun(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 9000.0f;

    // Generate Perlin noise
    float noiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Map noise range to color
    glm::vec3 sunColor = glm::vec3(1.0f, 0.5f, 0.0f) + noiseValue * 0.7f;

    // Set final fragment color
    color = Color(sunColor.r, sunColor.g, sunColor.b);

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment earth(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 900.0f;

    // Generate noise for the land and ocean
    float landNoiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Map noise to colors
    glm::vec3 landColor = glm::vec3(0.44f, 0.51f, 0.33f);
    glm::vec3 oceanColor = glm::vec3(0.12f, 0.38f, 0.57f);
    glm::vec3 polarColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Color para los polos (blanco)
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);

    if (landNoiseValue < 0.3f) {
        color = Color(oceanColor.r, oceanColor.g, oceanColor.b);
    } else {
        color = Color(landColor.r, landColor.g, landColor.b);
    }

    // Agrega los colores de los polos
    if (uv.y < -0.41 || uv.y > 0.41) {
        color = Color(polarColor.r, polarColor.g, polarColor.b);
    }

    // Generate noise for clouds
    float cloudNoiseValue = noiseGenerator.GetNoise(uv.x * scale, uv.y * scale);

    // Añade las nubes transparentes
    if (cloudNoiseValue > 0.7f) {
        glm::vec3 blendedColor = mix(glm::vec3(color.r, color.g, color.b), cloudColor, 0.3f);
        color = Color(blendedColor.r, blendedColor.g, blendedColor.b);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment jupiter(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    float jupiterRadius = 0.5;
    float distance = length(uv);
    glm::vec3 jupiterColor = glm::vec3(0.6, 0.3, 0.1);
    float bands = sin(15.0 * distance);
    float bandsIntensity = (0.5 + 0.5 * bands);

    // Aplicar variaciones de color basadas en ruido
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 2000.0f;

    float colorNoiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);
    glm::vec3 finalColor = jupiterColor + glm::vec3(colorNoiseValue * 0.05);
    finalColor *= bandsIntensity;
    color = Color(finalColor.r, finalColor.g, finalColor.b);

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment moon(Fragment& fragment, float time) {
    Color color;

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float scale = 5000.0f;

    // Generate noise for the moon's surface
    float noiseValue = noiseGenerator.GetNoise(fragment.originalPos.x * scale, fragment.originalPos.y * scale);

    // Definir un umbral para identificar cráteres
    float craterThreshold = 0.01f;

    if (noiseValue < craterThreshold) {
        // Usar un color más claro para representar cráteres
        color = Color(0.9f, 0.9f, 0.9f);
    } else {
        // Usar un color uniforme para la superficie de la luna
        color = Color(0.8f, 0.8f, 0.8f);
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment planetaGaseoso(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    float planetaRocosoRadius = 0.5; // Ajusta este valor según tus necesidades
    float distance = length(uv);
    float altitud = sin(10.0 * distance + time);
    glm::vec3 planetaRocosoColor = glm::vec3(0.6, 0.3, 0.1); // Ajusta este valor según tus necesidades

    // Aplicar variaciones de color basadas en ruido
    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 2000.0f;

    float colorNoiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);
    glm::vec3 finalColor = planetaRocosoColor + glm::vec3(colorNoiseValue * 0.05) + glm::vec3(0.2 * altitud);
    color = Color(finalColor.r, finalColor.g, finalColor.b);

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment antiTierra(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    float offsetX = 10000.0f;
    float offsetY = 10000.0f;
    float scale = 900.0f;

    // Generate noise for the land and ocean
    float landNoiseValue = noiseGenerator.GetNoise((uv.x + offsetX) * scale, (uv.y + offsetY) * scale);

    // Colores alternativos para la anti-Tierra
    glm::vec3 landColor = glm::vec3(0.2f, 0.1f, 0.1f); // Tonalidades de rojo oscuro
    glm::vec3 oceanColor = glm::vec3(0.05f, 0.05f, 0.15f); // Tonalidades de azul oscuro
    glm::vec3 polarColor = glm::vec3(0.0f, 0.0f, 0.0f);  // Color para los polos (gris oscuro)
    glm::vec3 cloudColor = glm::vec3(0.3f, 0.3f, 0.3f); // Color de las nubes (gris más oscuro)

    if (landNoiseValue < 0.01f) {
        color = Color(oceanColor.r, oceanColor.g, oceanColor.b);
    } else {
        color = Color(landColor.r, landColor.g, landColor.b);
    }

    // Agrega los colores de los polos
    if (uv.y < -0.41 || uv.y > 0.41) {
        color = Color(polarColor.r, polarColor.g, polarColor.b);
    }

    // Genera ruido para las nubes
    float cloudNoiseValue = noiseGenerator.GetNoise(uv.x * scale, uv.y * scale);

    // Añade nubes grises en la anti-Tierra
    if (cloudNoiseValue > 0.6f) {
        Color blendedColor = color + Color(cloudColor.r, cloudColor.g, cloudColor.b);
        color = blendedColor * 0.5f;
    }

    fragment.color = color * fragment.intensity;

    return fragment;
}

Fragment planetaColor(Fragment& fragment, float time) {
    Color color;

    // Get UV coordinates
    glm::vec2 uv = glm::vec2(fragment.originalPos.x, fragment.originalPos.y);

    // Define una variedad de colores
    glm::vec3 color1 = glm::vec3(1.0f, 0.0f, 0.0f);  // Rojo
    glm::vec3 color2 = glm::vec3(0.0f, 1.0f, 0.0f);  // Verde
    glm::vec3 color3 = glm::vec3(0.0f, 0.0f, 1.0f);  // Azul
    glm::vec3 color4 = glm::vec3(1.0f, 1.0f, 0.0f);  // Amarillo
    glm::vec3 color5 = glm::vec3(1.0f, 0.0f, 1.0f);  // Magenta
    glm::vec3 color6 = glm::vec3(0.0f, 1.0f, 1.0f);  // Cian

    // Calcula un patrón basado en la posición UV
    float pattern = (sin(uv.x * 20.0f) * cos(uv.y * 20.0f) + 1.0f) * 0.5f;

    // Mezcla los colores según el patrón
    glm::vec3 finalColor = mix(
            mix(mix(color1, color2, pattern), mix(color3, color4, pattern), pattern),
            mix(color5, color6, pattern),
            pattern
    );

    // Aplica el color final
    color = Color(finalColor.r, finalColor.g, finalColor.b);

    fragment.color = color * fragment.intensity;

    return fragment;
}
