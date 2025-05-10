#ifndef _BRDF_DISPATCH_
#define _BRDF_DISPATCH_

#define QUALITY_HIGH 3
#define QUALITY_MEDIUM 2
#define QUALITY_LOW 1

#define QUALITY QUALITY_HIGH

#include "../../common/math.glsl"

#include "brdf_config.glsl"
#include "brdf_normal_distribution.glsl"
#include "brdf_visibility.glsl"
#include "brdf_fresnel.glsl" 
#include "brdf_diffuse.glsl" 

 /**
  * @brief Dispatches to the selected specular Normal Distribution Function (NDF).
  * @param roughness The roughness parameter.
  * @param NoH The dot product of the normal vector (N) and the half-vector (H).
  * @param h The half-vector (H).
  *
  * @return The value of the selected NDF.
  */
float brdf_distribution(float roughness, float NoH, const vec3 h) {
#if BRDF_SPECULAR_D == SPECULAR_D_GGX
    return D_GGX(roughness, NoH, h);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected specular Visibility (V) function.
 *
 * @param roughness The roughness parameter.
 * @param NoV The dot product of the normal vector (N) and the view vector (V).
 * @param NoL The dot product of the normal vector (N) and the light vector (L).
 *
 * @return The value of the selected Visibility function.
 */
float brdf_visibility(float roughness, float NoV, float NoL) {
#if BRDF_SPECULAR_V == SPECULAR_V_SMITH_GGX
    return V_SmithGGXCorrelated(roughness, NoV, NoL);
#elif BRDF_SPECULAR_V == SPECULAR_V_SMITH_GGX_FAST
    return V_SmithGGXCorrelated_Fast(roughness, NoV, NoL);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected specular Fresnel (F) function.
 *
 * @param f0 The base Fresnel reflectance (at 0 degrees angle of incidence).
 * @param LoH The dot product of the light vector (L) and the half-vector (H).
 *
 * @return The Fresnel reflectance as a vec3.
 */
vec3 brdf_fresnel(const vec3 f0, float LoH) {
#if BRDF_SPECULAR_F == SPECULAR_F_SCHLICK
#if QUALITY == QUALITY_LOW
    return F_Schlick(f0, LoH);
#else
    float f90 = saturate(dot(f0, vec3(50.0 * 0.33)));
    return F_Schlick(f0, f90, LoH);
#endif
#else
	return f0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected specular Fresnel (F) function with explicit f90.
 *
 * @param f0 The base Fresnel reflectance (at 0 degrees angle of incidence).
 * @param f90 The grazing Fresnel reflectance (at 90 degrees angle of incidence).
 * @param LoH The dot product of the light vector (L) and the half-vector (H).
 *
 * @return The Fresnel reflectance as a vec3.
 */
vec3 brdf_fresnel(const vec3 f0, const float f90, float LoH) {
#if BRDF_SPECULAR_F == SPECULAR_F_SCHLICK
#if QUALITY == QUALITY_LOW
    return F_Schlick(f0, LoH);
#else
    return F_Schlick(f0, f90, LoH);
#endif
#else
	return f0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Anisotropic Normal Distribution Function (NDF).
 *
 * @param at Roughness in the tangent direction.
 * @param ab Roughness in the bitangent direction.
 * @param ToH The dot product of the tangent vector (T) and the half-vector (H).
 * @param BoH The dot product of the bitangent vector (B) and the half-vector (H).
 * @param NoH The dot product of the normal vector (N) and the half-vector (H).
 *
 * @return The value of the selected Anisotropic NDF..
 */
float brdf_distributionAnisotropic(float at, float ab, float ToH, float BoH, float NoH) {
#if BRDF_ANISOTROPIC_D == SPECULAR_D_GGX_ANISOTROPIC
    return D_GGX_Anisotropic(at, ab, ToH, BoH, NoH);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Anisotropic Visibility (V) function.
 *
 * @param roughness The overall roughness (may or may not be used depending on V function).
 * @param at Roughness in the tangent direction.
 * @param ab Roughness in the bitangent direction.
 * @param ToV The dot product of the tangent vector (T) and the view vector (V).
 * @param BoV The dot product of the bitangent vector (B) and the view vector (V).
 * @param ToL The dot product of the tangent vector (T) and the light vector (L).
 * @param BoL The dot product of the bitangent vector (B) and the light vector (L).
 * @param NoV The dot product of the normal vector (N) and the view vector (V).
 * @param NoL The dot product of the normal vector (N) and the light vector (L).
 *
 * @return The value of the selected Anisotropic Visibility function.
 */
float brdf_visibilityAnisotropic(float roughness, float at, float ab,
    float ToV, float BoV, float ToL, float BoL, float NoV, float NoL) {
#if BRDF_ANISOTROPIC_V == SPECULAR_V_SMITH_GGX
    return V_SmithGGXCorrelated(roughness, NoV, NoL);
#elif BRDF_ANISOTROPIC_V == SPECULAR_V_GGX_ANISOTROPIC
    return V_SmithGGXCorrelated_Anisotropic(at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Clear Coat Normal Distribution Function (NDF).
 *
 * @param roughness The roughness parameter for the clear coat layer.
 * @param NoH The dot product of the clear coat normal vector (N) and the half-vector (H).
 * @param h The half-vector (H).
 *
 * @return The value of the selected Clear Coat NDF.
 */
float brdf_distributionClearCoat(float roughness, float NoH, const vec3 h) {
#if BRDF_CLEAR_COAT_D == SPECULAR_D_GGX
    return D_GGX(roughness, NoH, h);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Clear Coat Visibility (V) function.
 *
 * @param LoH The dot product of the light vector (L) and the half-vector (H).
 *
 * @return The value of the selected Clear Coat Visibility function.
 */
float brdf_visibilityClearCoat(float LoH) {
#if BRDF_CLEAR_COAT_V == SPECULAR_V_KELEMEN
    return V_Kelemen(LoH);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Cloth Normal Distribution Function (NDF).
 *
 * @param roughness The roughness parameter for the cloth layer.
 * @param NoH The dot product of the normal vector (N) and the half-vector (H).
 *
 * @return The value of the selected Cloth NDF.
 */
float brdf_distributionCloth(float roughness, float NoH) {
#if BRDF_CLOTH_D == SPECULAR_D_CHARLIE
    return D_Charlie(roughness, NoH);
#else
	return 0.0; // Default case, should not happen
#endif
}

/**
 * @brief Dispatches to the selected Cloth Visibility (V) function.
 *
 * @param NoV The dot product of the normal vector (N) and the view vector (V).
 * @param NoL The dot product of the normal vector (N) and the light vector (L).
 *
 * @return The value of the selected Cloth Visibility function.
 */
float brdf_visibilityCloth(float NoV, float NoL) {
#if BRDF_CLOTH_V == SPECULAR_V_NEUBELT
    return V_Neubelt(NoV, NoL);
#else
	return 0.0; // Default case, should not happen
#endif
}

 /**
  * @brief Dispatches to the selected diffuse BRDF implementation.
  *
  * @param roughness The roughness parameter (may or may not be used depending on diffuse model).
  * @param NoV The dot product of the normal vector (N) and the view vector (V).
  * @param NoL The dot product of the normal vector (N) and the light vector (L).
  * @param LoH The dot product of the light vector (L) and the half-vector (H).
  *
  * @return The value of the selected diffuse BRDF.
  */
float brdf_diffuse(float roughness, float NoV, float NoL, float LoH) {
#if BRDF_DIFFUSE == DIFFUSE_LAMBERT
    return Fd_Lambert();
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY
    return Fd_Burley(roughness, NoV, NoL, LoH);
#else
	return 0.0; // Default case, should not happen
#endif
}

#endif