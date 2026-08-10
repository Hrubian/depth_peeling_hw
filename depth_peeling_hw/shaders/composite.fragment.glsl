#version 430 core

// Fragment shader for compositing multiple depth peeled layers using back-to-front alpha blending

layout(binding = 0) uniform sampler2D u_layer0;
layout(binding = 1) uniform sampler2D u_layer1;
layout(binding = 2) uniform sampler2D u_layer2;
layout(binding = 3) uniform sampler2D u_layer3;
layout(binding = 4) uniform sampler2D u_layer4;
layout(binding = 5) uniform sampler2D u_layer5;
layout(binding = 6) uniform sampler2D u_layer6;
layout(binding = 7) uniform sampler2D u_layer7;

uniform int u_numLayers;

in vec2 texCoords;
out vec4 fragColor;

vec4 getLayerColor(int index) {
	switch (index) {
		case 0: return texture(u_layer0, texCoords);
		case 1: return texture(u_layer1, texCoords);
		case 2: return texture(u_layer2, texCoords);
		case 3: return texture(u_layer3, texCoords);
		case 4: return texture(u_layer4, texCoords);
		case 5: return texture(u_layer5, texCoords);
		case 6: return texture(u_layer6, texCoords);
		case 7: return texture(u_layer7, texCoords);
		default: return vec4(0.0);
	}
}

void main() {
	vec4 result = vec4(0.0);
	
	// Standard back-to-front alpha compositing
	for (int i = u_numLayers - 1; i >= 0; i--) {
		vec4 layerColor = getLayerColor(i);
		if (layerColor.a > 0.0) {
			// Standard "Over" operator: layerColor on top of accumulated result
			result.rgb = layerColor.rgb * layerColor.a + result.rgb * (1.0 - layerColor.a);
			result.a   = layerColor.a + result.a * (1.0 - layerColor.a);
		}
	}
	
	fragColor = vec4(result.rgb, clamp(result.a, 0.0, 1.0));
}
