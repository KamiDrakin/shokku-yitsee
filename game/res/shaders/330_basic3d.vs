#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec2 vertexTexCoord;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matView;
uniform vec4 colDiffuse;

// Output vertex attributes (to fragment shader)
out vec4 fragColor;
out vec2 fragTexCoord;

out vec3 frag_light_pos;
out vec3 frag_normal;
uniform vec3 light_src;

void main() {
  vec4 pos = mvp * vec4(vertexPosition, 1.0f);
  if (length(vertexNormal) != 0.0f) {
    frag_light_pos = (matView * vec4(light_src, 1.0f) - matView * matModel * vec4(vertexPosition, 1.0f)).xyz;
    frag_normal = (matView * vec4(normalize(vertexNormal), 0.0f)).xyz;
  }
  else {
    frag_light_pos = (matView * vec4(light_src, 1.0f) - matView * vec4(vertexPosition, 1.0f)).xyz;
    frag_normal = vec3(0.0f, 0.0f, 1.0f);
  }
  fragColor = vertexColor * colDiffuse;
  fragTexCoord = vertexTexCoord;
  gl_Position = pos;
}