#version 460

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform Uniforms {
  mat4 projection_camera_matrix;
  mat4 world_matrix;
  vec4 eye_pos;
  vec4 light_pos;
  vec4 base_color;
  float light_energy;
  float ambient;
} uniforms;

layout (location = 0) in vec3 input_position;
layout (location = 1) in vec3 input_normal;
layout (location = 0) out vec3 output_color;
out gl_PerVertex
{
    vec4 gl_Position;
};

vec3 eotf( vec3 v ) {
  return min( max( v / (v + 0.155 ) * 1.019, vec3( 0, 0, 0 ) ), vec3( 1, 1, 1 ) );
}

void main() {
  const vec4 local_pos = vec4( input_position.xyz, 1.0 );
  const vec4 pos = uniforms.world_matrix * local_pos;
  const vec3 normal = normalize( mat3( uniforms.world_matrix) * input_normal );
  
  const float pi = 3.141592653589793;
  const vec3 light_dir = normalize( uniforms.light_pos.xyz - pos.xyz );
  const float diffuse = max( dot( light_dir, normal ), 0 ) /pi;
  output_color = eotf( ( uniforms.base_color.xyz * ( diffuse + uniforms.ambient ) ) * uniforms.light_energy );
  
  gl_Position = uniforms.projection_camera_matrix * pos;
}

