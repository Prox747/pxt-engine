#include "application.hpp"

#include "core/memory.hpp"
#include "core/events/event_dispatcher.hpp"
#include "core/events/window_event.hpp"
#include "core/constants.hpp"
#include "scene/ecs/component.hpp"
#include "scene/ecs/entity.hpp"
#include "scene/camera.hpp"
#include "graphics/render_systems/simple_render_system.hpp"
#include "graphics/render_systems/point_light_system.hpp"
#include "graphics/resources/image.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <chrono>

// IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imconfig.h"
#include "imgui_tables.cpp"
#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "imgui_demo.cpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace PXTEngine {

    Application* Application::Instance = nullptr;

    Application::Application() {
        Instance = this;

		std::vector<PoolSizeRatio> ratios = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5.0f},
		};

		m_descriptorAllocator = createUnique<DescriptorAllocatorGrowable>(m_context, SwapChain::MAX_FRAMES_IN_FLIGHT, ratios);

        m_imGuiPool = DescriptorPool::Builder(m_context)
            .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();
        // to enable imGui functionality
        initImGui();
    }

    Application::~Application() {
            for (auto& [_, system] : m_systems) {
                system->onShutdown();
                delete system;
            }

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        };

    void Application::initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplGlfw_InitForVulkan(m_window.getBaseWindow(), true);
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_context.getInstance();
        initInfo.PhysicalDevice = m_context.getPhysicalDevice();
        initInfo.Device = m_context.getDevice();
        initInfo.QueueFamily = m_context.findPhysicalQueueFamilies().graphicsFamily;
        initInfo.Queue = m_context.getGraphicsQueue();
        initInfo.RenderPass = m_renderer.getSwapChainRenderPass();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = m_imGuiPool->getDescriptorPool();
        initInfo.Allocator = nullptr;
        initInfo.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
        initInfo.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&initInfo);

        ImGui_ImplVulkan_CreateFontsTexture();
    }

    void Application::run() {
        std::vector<Unique<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            uboBuffers[i] = createUnique<Buffer>(
                m_context,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            uboBuffers[i]->map();
        }

        std::vector<std::string> textures_name = {
            "white_pixel.png",
            "shrek_420x420.png",
            "texture.jpg",
        };

        std::vector<Unique<Image>> textures;
        for (const auto& texture_name : textures_name) {
            textures.push_back(createUnique<Image>(TEXTURES_PATH + texture_name, m_context));
        }

        std::vector<VkDescriptorImageInfo> imageInfos;
        for (const auto& texture : textures) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture->getImageView();
            imageInfo.sampler = texture->getImageSampler();
            imageInfos.push_back(imageInfo);
        }

        auto globalSetLayout = DescriptorSetLayout::Builder(m_context)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textures.size())
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();

			m_descriptorAllocator->allocate(globalSetLayout->getDescriptorSetLayout(), globalDescriptorSets[i]);

            DescriptorWriter(m_context, *globalSetLayout)
                .writeBuffer(0, &bufferInfo)
                .writeImages(1, imageInfos.data(), imageInfos.size())
                .updateSet(globalDescriptorSets[i]);
        }

        m_window.setEventCallback([this](auto&& PH1) {
	        onEvent(std::forward<decltype(PH1)>(PH1));
        });

        SimpleRenderSystem simpleRenderSystem{
            m_context,
            m_renderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };

        PointLightSystem pointLightSystem{
            m_context,
            m_renderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };

        Camera camera;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
    
        m_scene.onStart();
        
        while (isRunning()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;
            
            m_scene.onUpdate(elapsedTime);
            
            float aspect = m_renderer.getAspectRatio();

            Entity mainCameraEntity = m_scene.getMainCameraEntity();

            if (mainCameraEntity) {
                const auto& cameraComponent = mainCameraEntity.get<CameraComponent>();
                const auto& transform = mainCameraEntity.get<TransformComponent>();

                camera = cameraComponent.camera;
                camera.setViewYXZ(transform.translation, transform.rotation);

                camera.setPerspective(glm::radians(50.f), aspect, 0.1f, 100.f);
            }
            
            if (auto commandBuffer = m_renderer.beginFrame()) {
                int frameIndex = m_renderer.getFrameIndex();

                FrameInfo frameInfo = {
                    frameIndex,
                    elapsedTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    m_scene
                };

                // update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjectionMatrix();
                ubo.view = camera.getViewMatrix();
                ubo.inverseView = camera.getInverseViewMatrix();

                // update light values into ubo
                pointLightSystem.update(frameInfo, ubo);

                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();
                
                // its not used yet
                for (auto& [_, system] : m_systems) {
                    system->onUpdate(elapsedTime);
                }

                // render 
                m_renderer.beginSwapChainRenderPass(commandBuffer);

                simpleRenderSystem.render(frameInfo);
                pointLightSystem.render(frameInfo);

                imGuiRenderUI(frameInfo);

                m_renderer.endSwapChainRenderPass(commandBuffer);
                m_renderer.endFrame();
            }
        }

        vkDeviceWaitIdle(m_context.getDevice());
    }

    bool Application::isRunning() {
        return !m_window.shouldClose() && m_running;
    }

    void Application::onEvent(Event& event) {
        EventDispatcher dispatcher(event);

        dispatcher.dispatch<WindowCloseEvent>([this](auto& event) {
            m_running = false;
        });

        for (auto& [_, system] : m_systems) {
            if (event.isHandled()) {
                break;
            }

            system->onEvent(event);
        }
    }

    void Application::imGuiRenderUI(FrameInfo& frameInfo) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameInfo.commandBuffer);
    }

    Entity Application::createPointLight(float intensity, float radius, glm::vec3 color) {
        Entity entity = m_scene.createEntity("point_light")
            .add<PointLightComponent>(intensity)
            .add<TransformComponent>(glm::vec3{0.f, 0.f, 0.f}, glm::vec3{radius, 1.f, 1.f}, glm::vec3{0.0f, 0.0f, 0.0f})
            .add<ColorComponent>(color);

        return entity;
    }
    
}

int main() {

    try {
        auto app = PXTEngine::initApplication();

        app->run();

        delete app;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}