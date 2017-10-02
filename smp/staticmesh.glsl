#version 330 core
#stage vertex
layout(location = 0) in vec3 vposition;
layout(location = 1) in vec2 vtexcoords;
layout(location = 2) in vec3 vnormal;
uniform mat4 model;
out vec2 ftexcoords;
out vec3 fnormal;

void main() {
  ftexcoords = vtexcoords;
  fnormal = normalize(mat3(model) * vnormal);
  gl_Position = viewProj * model * vec4(vposition, 1.f);
}

#stage fragment
layout(location = 0) out vec3 ocolor;
layout(location = 1) out vec3 onormal;
uniform sampler2D tex;
in vec2 ftexcoords;
in vec3 fnormal;

void main() {
  vec4 color = texture(tex,ftexcoords);
  if(alpha(color.a))
    discard;
  ocolor = color.rgb;
  onormal.xy = encodeNormal(fnormal);
}