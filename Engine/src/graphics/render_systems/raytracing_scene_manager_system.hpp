#pragma once

#include "graphics/resources/material_registry.hpp"
#include "graphics/resources/blas_registry.hpp"
#include "graphics/resources/vk_buffer.hpp"
#include "graphics/frame_info.hpp"
#include "graphics/descriptors/descriptors.hpp"

namespace PXTEngine {
	struct alignas(16) MeshInstanceData {
		VkDeviceAddress vertexBufferAddress;		// offset 0, size 8
		VkDeviceAddress indexBufferAddress;			// offset 8, size 8
		uint32_t materialIndex;						// offset 16, size 4
		float textureTilingFactor;					// offset 20, size 4
													// offset 24 -> 8 bit padding 
		alignas(16) glm::vec4 textureTintColor;		// offset 32, size 16
	};

	class RayTracingSceneManagerSystem {
	public:
		RayTracingSceneManagerSystem(Context& context, MaterialRegistry& materialRegistry, BLASRegistry& blasRegistry, 
			Shared<DescriptorAllocatorGrowable> allocator);
		~RayTracingSceneManagerSystem();

		// Delete the copy constructor and copy assignment operator
		RayTracingSceneManagerSystem(const RayTracingSceneManagerSystem&) = delete;
		RayTracingSceneManagerSystem& operator=(const RayTracingSceneManagerSystem&) = delete;

		void createTLAS(FrameInfo& frameInfo);
		void updateTLAS() {} // to implement later
		VkDescriptorSet getTLASDescriptorSet() const { return m_tlasDescriptorSet; }
		VkDescriptorSetLayout getTLASDescriptorSetLayout() const { return m_tlasDescriptorSetLayout->getDescriptorSetLayout(); }

		VkDescriptorSet getMeshInstanceDescriptorSet() const { return m_meshInstanceDescriptorSet; }
		VkDescriptorSetLayout getMeshInstanceDescriptorSetLayout() const { return m_meshInstanceDescriptorSetLayout->getDescriptorSetLayout(); }
	private:
		void destroyTLAS();
		VkTransformMatrixKHR glmToVkTransformMatrix(const glm::mat4& glmMatrix);

		void createTLASDescriptorSet();
		void updateTLASDescriptorSet(VkAccelerationStructureKHR& newTlas);

		void createMeshInstanceDescriptorSet();
		void updateMeshInstanceDescriptorSet();

		Context& m_context;
		MaterialRegistry& m_materialRegistry;
		BLASRegistry& m_blasRegistry;

		VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
		Unique<VulkanBuffer> m_tlasBuffer;
		VkAccelerationStructureBuildSizesInfoKHR m_buildSizeInfo{};
		VkAccelerationStructureCreateInfoKHR m_createInfo{};

		Shared<DescriptorAllocatorGrowable> m_descriptorAllocator;
		Shared<DescriptorSetLayout> m_tlasDescriptorSetLayout = nullptr;
		VkDescriptorSet m_tlasDescriptorSet = VK_NULL_HANDLE;

		std::vector<MeshInstanceData> m_meshInstanceData;
		Shared<DescriptorSetLayout> m_meshInstanceDescriptorSetLayout = nullptr;
		Unique<VulkanBuffer> m_meshInstanceBuffer = nullptr;
		VkDescriptorSet m_meshInstanceDescriptorSet = VK_NULL_HANDLE;
	};
}