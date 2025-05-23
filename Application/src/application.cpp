﻿#include "pxtengine.h"

#include "camera_controller.hpp"
#include "rotating_light_controller.hpp"

#include <random>

using namespace PXTEngine;

class App : public Application {
public:
    App() : Application() {}

    void loadScene() override {
		std::random_device rd;
		std::mt19937 gen(rd());

		// Position offset in a cube: [-0.5, 0.5] on each axis
		std::uniform_real_distribution<float> posDist(-0.7f, 0.7f);

		// Uniform scale: [0.5, 1.0]
		std::uniform_real_distribution<float> scaleDist(0.35f, 1.0f);

		// Rotation in radians: [0, 2π]
		std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

        Entity camera = getScene().createEntity("camera")
            .add<TransformComponent>(glm::vec3{0.0f, -0.2f, -1.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{-glm::pi<float>()/4, 0.0f, 0.0f})
            .add<CameraComponent>();
        
        camera.addAndGet<ScriptComponent>().bind<CameraController>();

        auto& rm = getResourceManager();

        ImageInfo albedoInfo{};
        albedoInfo.format = RGBA8_SRGB;

        auto bunny = rm.get<Mesh>(MODELS_PATH + "bunny/bunny.obj");
        auto bunnyMaterial = Material::Builder()
            .setAlbedoMap(rm.get<Image>(MODELS_PATH + "bunny/terracotta.jpg", &albedoInfo))
            .setNormalMap(rm.get<Image>(NORMAL_PIXEL_LINEAR))
			.setAmbientOcclusionMap(rm.get<Image>(WHITE_PIXEL_LINEAR))
            .build();

        auto ground = rm.get<Mesh>(MODELS_PATH + "quad.obj");
        auto vase = rm.get<Mesh>(MODELS_PATH + "smooth_vase.obj");
		auto defaultMaterial = rm.get<Material>(DEFAULT_MATERIAL);

        auto shrek = rm.get<Image>(TEXTURES_PATH + "shrek_420x420.png", &albedoInfo);
        auto texture = rm.get<Image>(TEXTURES_PATH + "texture.jpg", &albedoInfo);
        auto barrelBase = rm.get<Image>(TEXTURES_PATH + "barrel/barrel.png", &albedoInfo);
        auto barrelNormal = rm.get<Image>(TEXTURES_PATH + "barrel/barrel_normal.png");
        auto wallStoneBase = rm.get<Image>(TEXTURES_PATH + "wall_stone/base.png", &albedoInfo);
        auto wallStoneNormal = rm.get<Image>(TEXTURES_PATH + "wall_stone/normal.png");
        auto wallStoneRoughness = rm.get<Image>(TEXTURES_PATH + "wall_stone/roughness.png");
        auto wallStoneAO = rm.get<Image>(TEXTURES_PATH + "wall_stone/ambient_occlusion.png");
        auto stylizedStoneBase = rm.get<Image>(TEXTURES_PATH + "stylized_stone/base.png", &albedoInfo);
        auto stylizedStoneNormal = rm.get<Image>(TEXTURES_PATH + "stylized_stone/normal.png");
		auto stylizedStoneRoughness = rm.get<Image>(TEXTURES_PATH + "stylized_stone/roughness.png");
        auto stylizedStoneAO = rm.get<Image>(TEXTURES_PATH + "stylized_stone/ambient_occlusion.png");

		auto stylizedStoneMaterial = Material::Builder()
			.setAlbedoMap(stylizedStoneBase)
			.setNormalMap(stylizedStoneNormal)
			.setRoughnessMap(stylizedStoneRoughness)
			.setAmbientOcclusionMap(stylizedStoneAO)
			.build();

        Entity entity = getScene().createEntity("Floor")
            .add<TransformComponent>(glm::vec3{0.f, 1.f, 0.f}, glm::vec3{15.f, 15.f, 15.f}, glm::vec3{0.0f, 0.0f, 0.0f})
            .add<MeshComponent>(ground)
			.add<MaterialComponent>(MaterialComponent::Builder()
				.setMaterial(stylizedStoneMaterial)
				.setTilingFactor(50.0f)
				.build());
#if 0
        entity = getScene().createEntity("Roof")
            .add<TransformComponent>(glm::vec3{0.f, -1.0f, 0.f}, glm::vec3{1.f, 1.f, 1.f}, glm::vec3{ glm::pi<float>(), 0.0f, 0.0f})
            .add<ModelComponent>(quad);

        entity = getScene().createEntity("BackWall")
            .add<TransformComponent>(glm::vec3{0.0f, 0.f, 1.f}, glm::vec3{1.f, 1.f, 1.f}, glm::vec3{glm::pi<float>()/2, 0.0f, 0.0f})
            .add<ModelComponent>(quad);
        
        entity = getScene().createEntity("LeftWall")
            .add<TransformComponent>(glm::vec3{ -1.0f, 0.f, 0.0f }, glm::vec3{ 1.f, 1.f, 1.f }, glm::vec3{ glm::pi<float>() / 2, -glm::pi<float>() / 2, 0.0f })
            .add<ModelComponent>(quad);

        entity = getScene().createEntity("RightWall")
            .add<TransformComponent>(glm::vec3{ 1.0f, 0.f, 0.0f }, glm::vec3{ 1.f, 1.f, 1.f }, glm::vec3{ -glm::pi<float>() / 2, -glm::pi<float>() / 2, 0.0f })
            .add<ModelComponent>(quad);
#endif

        glm::vec4 colorWhite = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 colorGold = glm::vec4{1.0f, 0.843f, 0.0f, 1.0f};
        entity = getScene().createEntity("Bunny")
            .add<TransformComponent>(glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 2.5f, 2.5f, 2.5f }, glm::vec3{ glm::pi<float>(), 0.0f, 0.0f })
            .add<MeshComponent>(bunny)
            .add<MaterialComponent>(MaterialComponent::Builder()
                .setMaterial(bunnyMaterial)
                .build());


        // Vase
		for (int i = 0; i < 5; i++) {
			glm::vec3 pos = { posDist(gen), posDist(gen), posDist(gen) };
			float uniformScale = scaleDist(gen);
			glm::vec3 scale = { uniformScale, uniformScale, uniformScale };
			glm::vec3 rotation = { rotDist(gen), rotDist(gen), rotDist(gen) };

            entity = getScene().createEntity("vase" + std::to_string(i))
                .add<TransformComponent>(pos, scale, rotation)
                .add<MeshComponent>(vase);

			entity.addAndGet<MaterialComponent>().tint = glm::vec3(0.1f, 0.3f, 0.9f);
		}

        //entity = createPointLight(0.25f, 0.02f, glm::vec3{1.f, 1.f, 1.f});
        //entity.get<TransformComponent>().translation = glm::vec3{0.0f, 0.0f, 0.0f};

        // Three rotating lights (white, green, blue)
        entity = createPointLight(0.05f, 0.025f, glm::vec3{1.f, 1.f, 1.f});
        entity.get<TransformComponent>().translation = glm::vec3{10.0f / (float) sqrt(3), 0.5f, 0.2f};
        entity.addAndGet<ScriptComponent>().bind<RotatingLightController>();
#if 0
        entity = createPointLight(0.1f, 0.025f, glm::vec3{0.f, 1.f, 0.f});
        entity.get<TransformComponent>().translation = glm::vec3{-1.0f / (float) (2.0f * sqrt(3)), 0.2f, 0.5f};
        entity.addAndGet<ScriptComponent>().bind<RotatingLightController>();

        entity = createPointLight(0.1f, 0.025f, glm::vec3{0.f, 0.f, 1.f});
        entity.get<TransformComponent>().translation = glm::vec3{-1.0f / (float) (2.0f * sqrt(3)), 0.2f, -0.5f};
        entity.addAndGet<ScriptComponent>().bind<RotatingLightController>();
#endif
    }

    
};

PXTEngine::Application* PXTEngine::initApplication() {
    return new App();
}