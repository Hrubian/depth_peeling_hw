#version 430 core

// Simple passthrough fragment shader

layout(binding = 0) uniform sampler2D u_texture;

in vec2 texCoords;
out vec4 fragColor;

void main() {
	fragColor = texture(u_texture, texCoords);
}
