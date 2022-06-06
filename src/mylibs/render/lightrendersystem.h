#pragma once

#include <math/vector3.h>
#include <myvulkan/shader.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>

class LightRenderSystem
{
public:
    ~LightRenderSystem();

    bool init();

    bool updateReadTargets(const MeshRenderTargets &meshRenderTargets, const LightingRenderTargets& lightingRenderTargets);


    void update();

    void render(uint32_t width, uint32_t height);

    void setSunDirection(const Vec3& sunDir);

private:
    Vec3 sunDir{ 0.0f, 1.0f, 0.0f };

    UniformBufferHandle lightBufferHandle;

    Pipeline lightComputePipeline;
    VkSampler shadowTextureSampler = nullptr;
};