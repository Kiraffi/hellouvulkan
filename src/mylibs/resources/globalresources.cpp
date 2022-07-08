#include "globalresources.h"

#include <core/file.h>
#include <core/json.h>
#include <core/timer.h>

#include <model/gltf.h>

#include <scene/gameentity.h>


GlobalResources *globalResources = nullptr;

static bool loadModelForScene(Vector<GltfModel> &models, const char *filename, EntityType entityType)
{
    if(uint32_t(entityType) >= uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return false;
    GltfModel &gltfModel = models[uint32_t(entityType)];

    bool readSuccess = readGLTF(filename, gltfModel);

    printf("%s gltf read success: %i\n", filename, readSuccess);
    if(!readSuccess)
    {
        models.removeIndex(models.size() - 1);
        defragMemory();
        return false;
    }

    for(const auto &mesh : gltfModel.modelMeshes)
        printf("file: %s has %s model.\n", filename, mesh.meshName.getStr());

    for(const auto &animationName : gltfModel.animNames)
        printf("file: %s has %s animation.\n", filename, animationName.getStr());

    defragMemory();

    return true;
}

bool readAssets()
{
    String buffer;
    const char *assetStr = "assets/assets.json";
    if(!loadBytes(assetStr, buffer))
        return false;

    JsonBlock json;
    bool parseSuccess = json.parseJson(StringView(buffer.data(), buffer.size()));

    if(!parseSuccess)
    {
        printf("Failed to parse: %s\n", assetStr);
        return false;
    }
    else
    {
        //json.print();
    }

    return true;
}


bool initGlobalResources()
{
    globalResources = new GlobalResources();

    ScopedTimer timer("Scene init");
    globalResources->models.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES) + 1);
    globalResources->models[uint32_t(EntityType::NUM_OF_ENTITY_TYPES)].modelMeshes.push_back(GltfModel::ModelMesh());
    // load animated, can be loaded in any order nowadays, since animated vertices has separate buffer from non-animated ones
    if(!loadModelForScene(globalResources->models, "assets/models/animatedthing.gltf", EntityType::WOBBLY_THING))
        return false;
    if(!loadModelForScene(globalResources->models, "assets/models/character8.gltf", EntityType::CHARACTER))
        return false;
    if(!loadModelForScene(globalResources->models, "assets/models/lowpoly6.gltf", EntityType::LOW_POLY_CHAR))
        return false;
    if(!loadModelForScene(globalResources->models, "assets/models/armature_test.gltf", EntityType::ARMATURE_TEST))
        return false;
    if(!loadModelForScene(globalResources->models, "assets/models/character4_22.gltf", EntityType::NEW_CHARACTER_TEST))
        return false;



    // load nonanimated.
    if(!loadModelForScene(globalResources->models, "assets/models/arrows.gltf", EntityType::ARROW))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/test_gltf.gltf", EntityType::TEST_THING))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/tree1.gltf", EntityType::TREE))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/tree1_smooth.gltf", EntityType::TREE_SMOOTH))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/blob.gltf", EntityType::BLOB))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/blob_flat.gltf", EntityType::BLOB_FLAT))
        return false;

    if(!loadModelForScene(globalResources->models, "assets/models/floor.gltf", EntityType::FLOOR))
        return false;

    //return readAssets();

    return true;
}

void deinitGlobalResources()
{
    if(globalResources)
        delete globalResources;

    globalResources = nullptr;
}
