#include "graphics/descriptors/descriptor_writer.hpp"

#include <cassert>
#include <stdexcept>

namespace PXTEngine {    
    DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
        : m_setLayout{setLayout}, m_pool{pool} {}

    void DescriptorWriter::build(VkDescriptorSet& set) {
        m_pool.allocateDescriptorSet(m_setLayout.getDescriptorSetLayout(), set);
       
        overwrite(set);
    }

    void DescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : m_writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(m_pool.m_context.getDevice(), m_writes.size(), m_writes.data(), 0, nullptr);
    }

}