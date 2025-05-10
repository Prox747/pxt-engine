#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "ubo/global_ubo.glsl"
#include "material/surface_normal.glsl"
#include "lighting/blinn_phong_lighting.glsl"
#include "lighting/shadow_map.glsl"
#include "material/pbr/brdf.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in mat3 fragTBN;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];
layout(set = 2, binding = 0) uniform samplerCube shadowCubeMap;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec4 color;
    float specularIntensity;
    float shininess;
    int textureIndex;
    int normalMapIndex;
    int ambientOcclusionMapIndex;
    int metallicMapIndex;
	int roughnessMapIndex;
    float tilingFactor;
} push;

/*
 * Applies ambient occlusion to the given color using the ambient occlusion map.
 */
void applyAmbientOcclusion(inout vec3 color, vec2 texCoords) {
    float ao = texture(textures[push.ambientOcclusionMapIndex], texCoords).r;
    color *= ao;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 calculatePBRLighting(vec3 albedo, float metallic, float perceptualRoughness, vec3 N, vec3 V, vec2 texCoords) {
    // f0 -> Base reflectance at normal incidence
    // For dielectrics, f0 is typically vec3(0.04)
    // For metals, f0 is equal to the albedo
    // Metalness interpolates between these two cases
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    float roughness = perceptualRoughness * perceptualRoughness;

    float NoV = max(dot(N,V), 0.0);
	           
    vec3 reflectance = vec3(0.0);
    for(int i = 0; i < ubo.numLights; ++i) 
    {
        PointLight light = ubo.pointLights[i];

        // calculate per-light radiance
        vec3 L = normalize(light.position.xyz - fragPosWorld);
        vec3 H = normalize(V + L);
        float lightDistance = dot(L, L);
        float attenuation = 1.0 / (lightDistance * lightDistance);
        vec3 radiance = light.color.xyz * light.color.w * attenuation;   
        
        float NoL = max(dot(N,L), 0.0);
        float NoH = max(dot(N,H), 0.0);
        float LoH = max(dot(L,H), 0.0);
        
        float NDF = brdf_distribution(roughness, NoH, H);        
        float G = brdf_visibility(roughness, NoV, NoL);    
        vec3 F = brdf_fresnel(F0, LoH);       
        
        // kS = F, kS + kD = 1
        vec3 kD = vec3(1.0) - F;
        kD *= 1.0 - metallic;	  
        
        vec3 specular = NDF * G * F; 
        vec3 diffusion = kD * albedo * brdf_diffuse(roughness, NoV, NoL, LoH);
                            
        reflectance += (diffusion + specular) * radiance * NoL; 
    }   
  
    vec3 ambient = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w * albedo;
    applyAmbientOcclusion(ambient, texCoords);

    return ambient + reflectance;
}

void main() {
    vec2 texCoords = fragUV * push.tilingFactor;

    vec3 surfaceNormal = calculateSurfaceNormal(textures[push.normalMapIndex], texCoords, fragTBN);

    vec3 cameraPosWorld = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    vec3 albedo = texture(textures[push.textureIndex], texCoords).rgb;
    float metalness = texture(textures[push.metallicMapIndex], texCoords).r;
    float roughness = texture(textures[push.roughnessMapIndex], texCoords).r;

    vec3 color = calculatePBRLighting(albedo, metalness, roughness, surfaceNormal, viewDirection, texCoords);

    //applyAmbientOcclusion(albedo, texCoords);
    
    float shadow = computeShadowFactor(shadowCubeMap, surfaceNormal, fragPosWorld);

    outColor = vec4(color * shadow, 1.0);
}
