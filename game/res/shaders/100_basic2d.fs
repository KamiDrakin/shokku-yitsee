#version 100

precision highp float;

// Input vertex attributes (from vertex shader)
varying vec4 fragColor;

uniform vec3 color_depth;
/*
float my_round(float n) {
  float nx = n - floor(n);
  float ny;
  if (nx < 0.5)
    ny = 0.0;
  else
    ny = 1.0;
  return n - nx + ny;
}
*/
void main() {
  gl_FragColor = vec4(
    floor(fragColor.r * fragColor.a * color_depth.r) / color_depth.r,
    floor(fragColor.g * fragColor.a * color_depth.g) / color_depth.g,
    floor(fragColor.b * fragColor.a * color_depth.b) / color_depth.b,
    1.0
  );
}