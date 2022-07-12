
#include "podvectortype.h"
#include "podvectortypedefine.h"
#include "podvector.h"

#include <components/transform.h>

#include <container/stackstring.h>

#include <core/json.h>
#include <core/mytypes.h>


#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <model/animation.h>
#include <model/gltf.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

#include <render/font_render.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>

#include <scene/gameentity.h>

template void isPodType<char>();
template void isPodType<uint8_t>();
template void isPodType<int8_t>();
template void isPodType<uint16_t>();
template void isPodType<int16_t>();
template void isPodType<uint32_t>();
template void isPodType<int32_t>();
template void isPodType<uint64_t>();
template void isPodType<int64_t>();
template void isPodType<float>();
template void isPodType<double>();
template void isPodType<uint32_t const*>();

template void isPodType<Vector2>();
template void isPodType<Vector3>();
template void isPodType<Vector4>();
template void isPodType<Quaternion>();
template void isPodType<Mat3x4>();
template void isPodType<Matrix>();
template void isPodType<Transform>();

template void isPodType<GltfModel::AnimationVertex>();
template void isPodType<GltfModel::Vertex>();
template void isPodType<GltfModel::AnimPos>();
template void isPodType<GltfModel::AnimRot>();
template void isPodType<GltfModel::AnimScale>();
template void isPodType<GltfModel::AnimationIndexData>();

template void isPodType<DescriptorInfo>();
template void isPodType<DescriptorSetLayout>();
template void isPodType<RenderTarget>();
template void isPodType<Image>();
template void isPodType<Shader>();
template void isPodType<FontRenderSystem::GPUVertexData>();
template void isPodType<MeshRenderSystem::ModelData>();
template void isPodType<LineRenderSystem::Line>();

template void isPodType<TinyStackString>();
template void isPodType<SmallStackString>();
template void isPodType<MediumStackString>();
template void isPodType<LongStackString>();

template void isPodType<GameEntity>();

template void isPodType<AnimationState>();

template void isPodType<VkDescriptorPoolSize>();
template void isPodType<VkWriteDescriptorSet>();
template void isPodType<VkDescriptorBufferInfo>();
template void isPodType<VkImageView>();
template void isPodType<VkDescriptorSet>();
template void isPodType<VkMemoryBarrier>();
template void isPodType<VkBufferMemoryBarrier>();
template void isPodType<VkDescriptorImageInfo>();
template void isPodType<VkPipelineColorBlendAttachmentState>();
template void isPodType<VkClearValue>();
template void isPodType<VkAttachmentDescription>();
template void isPodType<VkPipelineShaderStageCreateInfo>();











template void newInPlace<JsonBlock>(JsonBlock *ptr, const JsonBlock &value);
template void newInPlace<GltfModel>(GltfModel *ptr, const GltfModel &value);

template void newInPlace<PodVector<uint32_t>>(PodVector<uint32_t> *ptr, const PodVector<uint32_t> &value);
template void newInPlace<PodVector<DescriptorInfo>>(PodVector<DescriptorInfo> *ptr, const PodVector<DescriptorInfo> &value);
template void newInPlace<PodVector<GltfModel::AnimationIndexData>>(PodVector<GltfModel::AnimationIndexData> *ptr, const PodVector<GltfModel::AnimationIndexData> &value);
template void newInPlace<PodVector<Mat3x4>>(PodVector<Mat3x4> *ptr, const PodVector<Mat3x4> &value);






template void isNotPodType<GltfModel>();
template void isNotPodType<GltfModel::ModelMesh>();
template void isNotPodType<JsonBlock>();

template void isNotPodType<PodVector<uint32_t>>();
template void isNotPodType<PodVector<DescriptorInfo>>();
template void isNotPodType<PodVector<GltfModel::AnimationIndexData>>();
template void isNotPodType<PodVector<Mat3x4>>();
