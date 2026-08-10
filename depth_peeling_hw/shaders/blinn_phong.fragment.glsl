#version 430 core

// Fragment shader with Blinn-Phong lighting model and depth peeling support

layout(binding = 0) uniform sampler2D u_diffuseTexture;
layout(binding = 1) uniform sampler2D u_prevDepthMap;

uniform mat4 u_modelMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;
uniform mat3 u_normalMat;
uniform vec3 u_viewPos;
uniform vec3 u_lightPos;

// Material properties for Blinn-Phong shading
uniform vec3 u_diffuseColor = vec3(0.8, 0.8, 0.8);
uniform vec3 u_specularColor = vec3(1.0, 1.0, 1.0);
uniform float u_shininess = 32.0;

// Light properties
uniform vec3 u_lightColor = vec3(1.0, 1.0, 1.0);
uniform float u_lightPower = 1.0;

// Ambient light
uniform vec3 u_ambientColor = vec3(0.1, 0.1, 0.1);

// Transparency
uniform float u_alpha = 1.0;

// Flag to use texture or color
uniform bool u_useTexture = false;

// Texture scaling for tiling
uniform vec2 u_texScale = vec2(1.0, 1.0);

// Depth peeling parameters
uniform bool u_usePeeling = false;
uniform vec2 u_screenSize = vec2(800.0, 600.0);

in vec3 f_normal;
in vec3 f_position;
in vec2 f_texCoord;

out vec4 fragColor;

void main() {
	// Perform depth peeling test if enabled (passes > 0)
	if (u_usePeeling) {
		vec2 screenUV = gl_FragCoord.xy / u_screenSize;
		float prevDepth = texture(u_prevDepthMap, screenUV).r;
		// If a previous layer was rendered at this pixel (prevDepth < 0.99999)
		// and current fragment depth is <= prevDepth + epsilon, discard it.
		if (prevDepth < 0.99999 && gl_FragCoord.z <= prevDepth + 0.00005) {
			discard;
		}
	}

	// Normalize the interpolated normal
	vec3 normal = normalize(f_normal);
	
	// Calculate directions
	vec3 lightDir = normalize(u_lightPos - f_position);
	vec3 viewDir = normalize(u_viewPos - f_position);
	
	// Get base color (either from texture or uniform)
	vec3 baseColor = u_diffuseColor;
	if (u_useTexture) {
		baseColor = texture(u_diffuseTexture, f_texCoord * u_texScale).rgb;
	}
	
	// Ambient component
	vec3 ambient = u_ambientColor * baseColor;
	
	// Diffuse component (Lambertian reflection)
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * baseColor * u_lightColor * u_lightPower;
	
	// Specular component (Blinn-Phong)
	vec3 specular = vec3(0.0);
	if (diff > 0.0) {
		vec3 halfway = normalize(lightDir + viewDir);
		float spec = max(dot(normal, halfway), 0.0);
		specular = pow(spec, u_shininess) * u_specularColor * u_lightColor * u_lightPower;
	}
	
	// Combine all components
	vec3 color = ambient + diffuse + specular;
	
	fragColor = vec4(color, u_alpha);
}
