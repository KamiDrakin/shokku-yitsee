#version 330

// Input vertex attributes (from vertex shader)
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

uniform vec3 color_depth;

void main() {
  finalColor = vec4(
    floor(fragColor.r * fragColor.a * color_depth.r) / color_depth.r,
    floor(fragColor.g * fragColor.a * color_depth.g) / color_depth.g,
    floor(fragColor.b * fragColor.a * color_depth.b) / color_depth.b,
    1.0f
  );
}