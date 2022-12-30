#pragma once

#include <math/vector3.h>
#include <myvulkan/shader.h>


struct LightingRenderTargets;
struct MeshRenderTargets;

class LightRenderSystem
{
public:
    static bool init();
    static void deinit();
    static bool updateReadTargets(const MeshRenderTargets& meshRenderTargets,
        const LightingRenderTargets& lightingRenderTargets);


    static void update();

    static void render(uint32_t width, uint32_t height);

    // Maybe the system should read the sun direction from some sort of lightmanager?
    static void setSunDirection(const Vec3& sunDir);

};