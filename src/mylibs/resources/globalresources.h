#pragma once

#include <container/vectorsbase.h>

#include <model/gltf.h>

struct GlobalResources
{
    Vector<GltfModel> models;

};

extern GlobalResources *globalResources;

bool initGlobalResources();
void deinitGlobalResources();
