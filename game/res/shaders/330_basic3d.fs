#version 330

// Input vertex attributes (from vertex shader)
in vec4 fragColor;
in vec2 fragTexCoord;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

in vec3 frag_light_pos;
in vec3 frag_normal;
uniform vec3 color_depth;
uniform float light_intensity;
uniform int with_texture;

vec4 apply_color_depth(vec4 v) {
  return vec4(
    floor(v.r * color_depth.r) / color_depth.r,
    floor(v.g * color_depth.g) / color_depth.g,
    floor(v.b * color_depth.b) / color_depth.b,
    1.0f
  );
}

void main() {
  float color_mul;
  vec4 uncomp_color;
  color_mul = (1.0f + gl_FragCoord.z) / 2.0f;
  color_mul = 1.0f - color_mul * color_mul;
  color_mul *= 0.3f + 0.7f * ((1.0f + dot(normalize(frag_normal), normalize(frag_light_pos))) / 2) * light_intensity / length(frag_light_pos);
  color_mul *= fragColor.a;
  color_mul = clamp(color_mul, 0.0f, 1.0f);
  if (with_texture == 0)
    uncomp_color = fragColor * color_mul;
  else {
    uncomp_color = fragColor * texture(texture0, fragTexCoord);
    if (uncomp_color.a < 1.0f)
      discard;
    uncomp_color = apply_color_depth(uncomp_color) * color_mul;
  }
  finalColor = vec4(apply_color_depth(uncomp_color).xyz * fragColor.a, 1.0f);
}