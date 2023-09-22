#version 100

precision highp float;

// Input vertex attributes (from vertex shader)
varying vec4 fragColor;
varying vec2 fragTexCoord;

// Input uniform values
uniform sampler2D texture0;

varying vec3 frag_light_pos;
varying vec3 frag_normal;
uniform vec3 color_depth;
uniform float light_intensity;
uniform int with_texture;
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
vec4 apply_color_depth(vec4 v) {
  return vec4(
    floor(v.r * color_depth.r) / color_depth.r,
    floor(v.g * color_depth.g) / color_depth.g,
    floor(v.b * color_depth.b) / color_depth.b,
    1.0
  );
}

void main() {
  float color_mul;
  vec4 uncomp_color;
  color_mul = (1.0 + gl_FragCoord.z) / 2.0;
  color_mul = 1.0 - color_mul * color_mul;
  color_mul *= 0.3 + 0.7 * ((1.0 + dot(frag_normal, normalize(frag_light_pos))) / 2.0) * light_intensity / length(frag_light_pos);
  color_mul *= fragColor.a;
  color_mul = clamp(color_mul, 0.0, 1.0);
  if (with_texture == 0)
    uncomp_color = fragColor * color_mul;
  else {
    uncomp_color = fragColor * texture2D(texture0, fragTexCoord);
    if (uncomp_color.a < 1.0)
      discard;
    uncomp_color = apply_color_depth(uncomp_color) * color_mul;
  }
  gl_FragColor = vec4(apply_color_depth(uncomp_color).xyz * fragColor.a, 1.0);
}

