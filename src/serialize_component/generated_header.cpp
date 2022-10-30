// This is generated file, do not modify.
#include "generated_header.h"

#include "src/components.h"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <atomic>
#include <mutex>
#include <vector>

bool TransformComponent::serialize(WriteJson &json) const
{
    json.addObject();
    json.addString("ComponentType", componentName);
    json.addInteger("ComponentTypeId", u32(componentID));
    json.addInteger("ComponentVersion", componentVersion);

    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
            return false;
    }
    json.endObject();
    return json.isValid();
}

bool TransformComponent::deserialize(const JsonBlock &json)
{
    if(!json.isObject() || !json.isValid())
        return false;

    if(!json.getChild("ComponentType").equals(componentName))
        return false;
    if(!json.getChild("ComponentTypeId").equals(u32(componentID)))
        return false;
    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
    }
    return true;
}

void* TransformComponent::getElementIndexRef(u32 index)
{
    switch(index)
    {
            case 0: return &position;
            case 1: return &rotation;
            case 2: return &scale;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}

const void* TransformComponent::getElementIndex(u32 index) const
{
    switch(index)
    {
            case 0: return &position;
            case 1: return &rotation;
            case 2: return &scale;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}


bool Mat4Component::serialize(WriteJson &json) const
{
    json.addObject();
    json.addString("ComponentType", componentName);
    json.addInteger("ComponentTypeId", u32(componentID));
    json.addInteger("ComponentVersion", componentVersion);

    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
            return false;
    }
    json.endObject();
    return json.isValid();
}

bool Mat4Component::deserialize(const JsonBlock &json)
{
    if(!json.isObject() || !json.isValid())
        return false;

    if(!json.getChild("ComponentType").equals(componentName))
        return false;
    if(!json.getChild("ComponentTypeId").equals(u32(componentID)))
        return false;
    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
    }
    return true;
}

void* Mat4Component::getElementIndexRef(u32 index)
{
    switch(index)
    {
            case 0: return &mat;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}

const void* Mat4Component::getElementIndex(u32 index) const
{
    switch(index)
    {
            case 0: return &mat;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}


bool Heritaged1::serialize(WriteJson &json) const
{
    json.addObject();
    json.addString("ComponentType", componentName);
    json.addInteger("ComponentTypeId", u32(componentID));
    json.addInteger("ComponentVersion", componentVersion);

    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
            return false;
    }
    json.endObject();
    return json.isValid();
}

bool Heritaged1::deserialize(const JsonBlock &json)
{
    if(!json.isObject() || !json.isValid())
        return false;

    if(!json.getChild("ComponentType").equals(componentName))
        return false;
    if(!json.getChild("ComponentTypeId").equals(u32(componentID)))
        return false;
    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
    }
    return true;
}

void* Heritaged1::getElementIndexRef(u32 index)
{
    switch(index)
    {
            case 0: return &tempInt;
            case 1: return &tempFloat;
            case 2: return &tempV2;
            case 3: return &tempV3;
            case 4: return &tempV4;
            case 5: return &tempQuat;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}

const void* Heritaged1::getElementIndex(u32 index) const
{
    switch(index)
    {
            case 0: return &tempInt;
            case 1: return &tempFloat;
            case 2: return &tempV2;
            case 3: return &tempV3;
            case 4: return &tempV4;
            case 5: return &tempQuat;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}


bool Heritaged2::serialize(WriteJson &json) const
{
    json.addObject();
    json.addString("ComponentType", componentName);
    json.addInteger("ComponentTypeId", u32(componentID));
    json.addInteger("ComponentVersion", componentVersion);

    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
            return false;
    }
    json.endObject();
    return json.isValid();
}

bool Heritaged2::deserialize(const JsonBlock &json)
{
    if(!json.isObject() || !json.isValid())
        return false;

    if(!json.getChild("ComponentType").equals(componentName))
        return false;
    if(!json.getChild("ComponentTypeId").equals(u32(componentID)))
        return false;
    for(u32 i = 0; i < componentFieldAmount; ++i)
    {
        deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
    }
    return true;
}

void* Heritaged2::getElementIndexRef(u32 index)
{
    switch(index)
    {
            case 0: return &tempInt;
            case 1: return &tempInt2;
            case 2: return &tempInt3;
            case 3: return &tempFloat2;
            case 4: return &tempMat3x4;
            case 5: return &tempMatrix;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}

const void* Heritaged2::getElementIndex(u32 index) const
{
    switch(index)
    {
            case 0: return &tempInt;
            case 1: return &tempInt2;
            case 2: return &tempInt3;
            case 3: return &tempFloat2;
            case 4: return &tempMat3x4;
            case 5: return &tempMatrix;
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
}



GameEntitySystem::GameEntitySystemComponentArrayHandleBuilder& GameEntitySystem::GameEntitySystemComponentArrayHandleBuilder::addComponent(ComponentType componentType)
{
    u32 componentIndex = GameEntitySystem::getComponentIndex(componentType);
    if(componentIndex >= GameEntitySystem::componentTypeCount)
    {
        ASSERT(componentIndex < GameEntitySystem::componentTypeCount);
        return *this;
    }
    componentIndexArray |= u64(1) << u64(componentIndex);
    return *this;
}

GameEntitySystem::~GameEntitySystem()
{
    syncReadWrites();
}

bool GameEntitySystem::hasComponent(EntitySystemHandle handle, ComponentType componentType) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[handle.entityIndex] >> componentIndex) & 1) == 1;
}

bool GameEntitySystem::hasComponents(EntitySystemHandle handle,
    const GameEntitySystemComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[handle.entityIndex] & compIndArr) == compIndArr;
}

bool GameEntitySystem::hasComponent(u32 entityIndex, ComponentType componentType) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[entityIndex] >> componentIndex) & 1) == 1;
}

bool GameEntitySystem::hasComponents(u32 entityIndex,
    const GameEntitySystemComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

bool GameEntitySystem::hasComponents(u32 entityIndex, const EntityRWHandle &rwHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    if(rwHandle.rwHandleTypeId != entitySystemID || rwHandle.syncIndexPoint != currentSyncIndex)
    {
        ASSERT(rwHandle.rwHandleTypeId == entitySystemID);
        ASSERT(rwHandle.syncIndexPoint == currentSyncIndex);
        return false;
    }

    u64 compIndArr = rwHandle.readArrays | rwHandle.writeArrays;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

u32 GameEntitySystem::getComponentIndex(ComponentType componentType)
{
    // Could be written with switch-cases if it comes to that. Probably no need to though
    for(u32 i = 0; i < componentTypeCount; ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
}

const EntityRWHandle GameEntitySystem::getRWHandle(
    const GameEntitySystemComponentArrayHandleBuilder& readBuilder,
    const GameEntitySystemComponentArrayHandleBuilder& writeBuilder)
{
    u64 reads = readArrays.fetch_or(readBuilder.componentIndexArray);
    u64 writes = writeArrays.fetch_or(writeBuilder.componentIndexArray);

    reads |= readBuilder.componentIndexArray;
    writes |= writeBuilder.componentIndexArray;

    ASSERT((writes & reads) == 0);
    if((writes & reads) == 0)
    {
        return EntityRWHandle{
            .readArrays = readBuilder.componentIndexArray,
            .writeArrays = writeBuilder.componentIndexArray,
            .syncIndexPoint = currentSyncIndex,
            .rwHandleTypeId = entitySystemID
       };
    }
    return EntityRWHandle{};
}

EntitySystemHandle GameEntitySystem::getEntitySystemHandle(u32 index) const
{
    if(index >= entityComponents.size())
        return EntitySystemHandle();

    return EntitySystemHandle {
        .entitySystemType = entitySystemID,
        .entityIndexVersion = entityVersions[index],
        .entityIndex = index };
}

GameEntitySystem::GameEntitySystemEntityLockedMutexHandle GameEntitySystem::getLockedMutexHandle()
{
    entityAddRemoveMutex.lock();
    ++mutexLockIndex;
    return GameEntitySystem::GameEntitySystemEntityLockedMutexHandle { .lockIndex = mutexLockIndex };
}

bool GameEntitySystem::releaseLockedMutexHandle(const GameEntitySystem::GameEntitySystemEntityLockedMutexHandle& handle)
{
    if(mutexLockIndex != handle.lockIndex)
        return false;

    ++mutexLockIndex;
    entityAddRemoveMutex.unlock();
    return true;
}

bool GameEntitySystem::syncReadWrites()
{
    u64 reads = readArrays.load();
    u64 writes = writeArrays.load();

    readArrays.store(u64(0));
    writeArrays.store(u64(0));
    ++currentSyncIndex;

    // Cannot have both reading and writing to same array in same sync point.
    ASSERT((reads & writes) == 0);

    bool readWrite = (reads | writes) != 0;
    bool addRemove = entitiesAdded || entitiesRemoved;
    ASSERT(!(readWrite && addRemove));

    entitiesAdded = false;
    entitiesRemoved = false;

    return ((reads & writes) == 0) && !(readWrite && addRemove);
}

EntitySystemHandle GameEntitySystem::addEntity(const GameEntitySystemEntityLockedMutexHandle& handle)
{
    u32 addIndex = ~0u;

    if(freeEntityIndices.size() == 0)
    {
        TransformComponentArray.emplace_back();
        Mat4ComponentArray.emplace_back();
        entityComponents.emplace_back(0);
        entityVersions.emplace_back(1);
        addIndex = entityComponents.size() - 1;

    }
    else
    {
        u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
        freeEntityIndices.resize(freeEntityIndices.size() - 1);
        entityComponents[freeIndex] = 0;
        ++entityVersions[freeIndex];
        addIndex = freeIndex;
    }
    entitiesAdded = true;
    return getEntitySystemHandle(addIndex);
}

bool GameEntitySystem::removeEntity(EntitySystemHandle handle, const GameEntitySystemEntityLockedMutexHandle &mutexHandle)
{
    ASSERT(mutexHandle.lockIndex == mutexLockIndex);
    if(mutexHandle.lockIndex != mutexLockIndex)
        return false;

    ASSERT(handle.entitySystemType == entitySystemID);
    if(handle.entitySystemType != entitySystemID)
        return false;

    ASSERT(handle.entityIndex < entityComponents.size());
    if(handle.entityIndex >= entityComponents.size())
        return false;

    ASSERT(handle.entityIndexVersion == entityVersions[handle.entityIndex]);
    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u32 freeIndex = handle.entityIndex;
    entityComponents[freeIndex] = 0;
    ++entityVersions[freeIndex];
    freeEntityIndices.emplace_back(freeIndex);

    entitiesRemoved = true;

    return true;
}

const TransformComponent* GameEntitySystem::getTransformComponentReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(TransformComponent::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return TransformComponentArray.data();
}
TransformComponent* GameEntitySystem::getTransformComponentWriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(TransformComponent::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return TransformComponentArray.data();
}
const Mat4Component* GameEntitySystem::getMat4ComponentReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Mat4Component::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Mat4ComponentArray.data();
}
Mat4Component* GameEntitySystem::getMat4ComponentWriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(Mat4Component::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Mat4ComponentArray.data();
}

bool GameEntitySystem::addTransformComponentComponent(EntitySystemHandle handle, const TransformComponent& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(TransformComponent::componentID);

    if(componentIndex >= componentTypeCount)
        return false;

    if(hasComponent(handle, TransformComponent::componentID))
        return false;

    TransformComponentArray[handle.entityIndex] = component;
    entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

    return true;
}

bool GameEntitySystem::addMat4ComponentComponent(EntitySystemHandle handle, const Mat4Component& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(Mat4Component::componentID);

    if(componentIndex >= componentTypeCount)
        return false;

    if(hasComponent(handle, Mat4Component::componentID))
        return false;

    Mat4ComponentArray[handle.entityIndex] = component;
    entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

    return true;
}

bool GameEntitySystem::serialize(WriteJson &json) const
{
    u32 entityAmount = entityComponents.size();
    if(entityAmount == 0)
        return false;

    json.addObject(entitySystemName);
    json.addString("EntityType", entitySystemName);
    json.addInteger("EntityTypeId", u32(entitySystemID));
    json.addInteger("EntityVersion", entityVersion);
    json.addArray("Entities");
    for(u32 i = 0; i < entityAmount; ++i)
    {
        json.addObject();
        json.addArray("Components");

            if(hasComponent(getEntitySystemHandle(i), TransformComponent::componentID))
            {
                TransformComponentArray[i].serialize(json);
            }
            if(hasComponent(getEntitySystemHandle(i), Mat4Component::componentID))
            {
                Mat4ComponentArray[i].serialize(json);
            }
        json.endArray();
        json.endObject();
    }
    json.endArray();
    json.endObject();
    return json.isValid();
}

bool GameEntitySystem::deserialize(const JsonBlock &json, const GameEntitySystemEntityLockedMutexHandle &mutexHandle)
{
    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    const JsonBlock& child = json.getChild(entitySystemName);
    if(!child.isValid())
        return false;

    if(!child.getChild("EntityTypeId").equals(u32(entitySystemID)) || !child.getChild("EntityType").equals(entitySystemName))
        return false;

    u32 addedCount = 0u;
    for(const auto &entityJson : child.getChild("Entities"))
    {
        addEntity(mutexHandle);
        for(const auto &obj : entityJson.getChild("Components"))
        {
                if(TransformComponentArray[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(TransformComponent::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
                if(Mat4ComponentArray[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Mat4Component::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
        }
        addedCount++;
    }
    return true;
}



StaticModelEntity::StaticModelEntityComponentArrayHandleBuilder& StaticModelEntity::StaticModelEntityComponentArrayHandleBuilder::addComponent(ComponentType componentType)
{
    u32 componentIndex = StaticModelEntity::getComponentIndex(componentType);
    if(componentIndex >= StaticModelEntity::componentTypeCount)
    {
        ASSERT(componentIndex < StaticModelEntity::componentTypeCount);
        return *this;
    }
    componentIndexArray |= u64(1) << u64(componentIndex);
    return *this;
}

StaticModelEntity::~StaticModelEntity()
{
    syncReadWrites();
}

bool StaticModelEntity::hasComponent(EntitySystemHandle handle, ComponentType componentType) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[handle.entityIndex] >> componentIndex) & 1) == 1;
}

bool StaticModelEntity::hasComponents(EntitySystemHandle handle,
    const StaticModelEntityComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[handle.entityIndex] & compIndArr) == compIndArr;
}

bool StaticModelEntity::hasComponent(u32 entityIndex, ComponentType componentType) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[entityIndex] >> componentIndex) & 1) == 1;
}

bool StaticModelEntity::hasComponents(u32 entityIndex,
    const StaticModelEntityComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

bool StaticModelEntity::hasComponents(u32 entityIndex, const EntityRWHandle &rwHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    if(rwHandle.rwHandleTypeId != entitySystemID || rwHandle.syncIndexPoint != currentSyncIndex)
    {
        ASSERT(rwHandle.rwHandleTypeId == entitySystemID);
        ASSERT(rwHandle.syncIndexPoint == currentSyncIndex);
        return false;
    }

    u64 compIndArr = rwHandle.readArrays | rwHandle.writeArrays;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

u32 StaticModelEntity::getComponentIndex(ComponentType componentType)
{
    // Could be written with switch-cases if it comes to that. Probably no need to though
    for(u32 i = 0; i < componentTypeCount; ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
}

const EntityRWHandle StaticModelEntity::getRWHandle(
    const StaticModelEntityComponentArrayHandleBuilder& readBuilder,
    const StaticModelEntityComponentArrayHandleBuilder& writeBuilder)
{
    u64 reads = readArrays.fetch_or(readBuilder.componentIndexArray);
    u64 writes = writeArrays.fetch_or(writeBuilder.componentIndexArray);

    reads |= readBuilder.componentIndexArray;
    writes |= writeBuilder.componentIndexArray;

    ASSERT((writes & reads) == 0);
    if((writes & reads) == 0)
    {
        return EntityRWHandle{
            .readArrays = readBuilder.componentIndexArray,
            .writeArrays = writeBuilder.componentIndexArray,
            .syncIndexPoint = currentSyncIndex,
            .rwHandleTypeId = entitySystemID
       };
    }
    return EntityRWHandle{};
}

EntitySystemHandle StaticModelEntity::getEntitySystemHandle(u32 index) const
{
    if(index >= entityComponents.size())
        return EntitySystemHandle();

    return EntitySystemHandle {
        .entitySystemType = entitySystemID,
        .entityIndexVersion = entityVersions[index],
        .entityIndex = index };
}

StaticModelEntity::StaticModelEntityEntityLockedMutexHandle StaticModelEntity::getLockedMutexHandle()
{
    entityAddRemoveMutex.lock();
    ++mutexLockIndex;
    return StaticModelEntity::StaticModelEntityEntityLockedMutexHandle { .lockIndex = mutexLockIndex };
}

bool StaticModelEntity::releaseLockedMutexHandle(const StaticModelEntity::StaticModelEntityEntityLockedMutexHandle& handle)
{
    if(mutexLockIndex != handle.lockIndex)
        return false;

    ++mutexLockIndex;
    entityAddRemoveMutex.unlock();
    return true;
}

bool StaticModelEntity::syncReadWrites()
{
    u64 reads = readArrays.load();
    u64 writes = writeArrays.load();

    readArrays.store(u64(0));
    writeArrays.store(u64(0));
    ++currentSyncIndex;

    // Cannot have both reading and writing to same array in same sync point.
    ASSERT((reads & writes) == 0);

    bool readWrite = (reads | writes) != 0;
    bool addRemove = entitiesAdded || entitiesRemoved;
    ASSERT(!(readWrite && addRemove));

    entitiesAdded = false;
    entitiesRemoved = false;

    return ((reads & writes) == 0) && !(readWrite && addRemove);
}

EntitySystemHandle StaticModelEntity::addEntity(const StaticModelEntityEntityLockedMutexHandle& handle)
{
    u32 addIndex = ~0u;

    if(freeEntityIndices.size() == 0)
    {
        Heritaged1Array.emplace_back();
        Heritaged2Array.emplace_back();
        entityComponents.emplace_back(0);
        entityVersions.emplace_back(1);
        addIndex = entityComponents.size() - 1;

    }
    else
    {
        u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
        freeEntityIndices.resize(freeEntityIndices.size() - 1);
        entityComponents[freeIndex] = 0;
        ++entityVersions[freeIndex];
        addIndex = freeIndex;
    }
    entitiesAdded = true;
    return getEntitySystemHandle(addIndex);
}

bool StaticModelEntity::removeEntity(EntitySystemHandle handle, const StaticModelEntityEntityLockedMutexHandle &mutexHandle)
{
    ASSERT(mutexHandle.lockIndex == mutexLockIndex);
    if(mutexHandle.lockIndex != mutexLockIndex)
        return false;

    ASSERT(handle.entitySystemType == entitySystemID);
    if(handle.entitySystemType != entitySystemID)
        return false;

    ASSERT(handle.entityIndex < entityComponents.size());
    if(handle.entityIndex >= entityComponents.size())
        return false;

    ASSERT(handle.entityIndexVersion == entityVersions[handle.entityIndex]);
    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u32 freeIndex = handle.entityIndex;
    entityComponents[freeIndex] = 0;
    ++entityVersions[freeIndex];
    freeEntityIndices.emplace_back(freeIndex);

    entitiesRemoved = true;

    return true;
}

const Heritaged1* StaticModelEntity::getHeritaged1ReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
Heritaged1* StaticModelEntity::getHeritaged1WriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
const Heritaged2* StaticModelEntity::getHeritaged2ReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged2Array.data();
}
Heritaged2* StaticModelEntity::getHeritaged2WriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged2Array.data();
}

bool StaticModelEntity::addHeritaged1Component(EntitySystemHandle handle, const Heritaged1& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(Heritaged1::componentID);

    if(componentIndex >= componentTypeCount)
        return false;

    if(hasComponent(handle, Heritaged1::componentID))
        return false;

    Heritaged1Array[handle.entityIndex] = component;
    entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

    return true;
}

bool StaticModelEntity::addHeritaged2Component(EntitySystemHandle handle, const Heritaged2& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(Heritaged2::componentID);

    if(componentIndex >= componentTypeCount)
        return false;

    if(hasComponent(handle, Heritaged2::componentID))
        return false;

    Heritaged2Array[handle.entityIndex] = component;
    entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

    return true;
}

bool StaticModelEntity::serialize(WriteJson &json) const
{
    u32 entityAmount = entityComponents.size();
    if(entityAmount == 0)
        return false;

    json.addObject(entitySystemName);
    json.addString("EntityType", entitySystemName);
    json.addInteger("EntityTypeId", u32(entitySystemID));
    json.addInteger("EntityVersion", entityVersion);
    json.addArray("Entities");
    for(u32 i = 0; i < entityAmount; ++i)
    {
        json.addObject();
        json.addArray("Components");

            if(hasComponent(getEntitySystemHandle(i), Heritaged1::componentID))
            {
                Heritaged1Array[i].serialize(json);
            }
            if(hasComponent(getEntitySystemHandle(i), Heritaged2::componentID))
            {
                Heritaged2Array[i].serialize(json);
            }
        json.endArray();
        json.endObject();
    }
    json.endArray();
    json.endObject();
    return json.isValid();
}

bool StaticModelEntity::deserialize(const JsonBlock &json, const StaticModelEntityEntityLockedMutexHandle &mutexHandle)
{
    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    const JsonBlock& child = json.getChild(entitySystemName);
    if(!child.isValid())
        return false;

    if(!child.getChild("EntityTypeId").equals(u32(entitySystemID)) || !child.getChild("EntityType").equals(entitySystemName))
        return false;

    u32 addedCount = 0u;
    for(const auto &entityJson : child.getChild("Entities"))
    {
        addEntity(mutexHandle);
        for(const auto &obj : entityJson.getChild("Components"))
        {
                if(Heritaged1Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
                if(Heritaged2Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
        }
        addedCount++;
    }
    return true;
}



OtherTestEntity::OtherTestEntityComponentArrayHandleBuilder& OtherTestEntity::OtherTestEntityComponentArrayHandleBuilder::addComponent(ComponentType componentType)
{
    u32 componentIndex = OtherTestEntity::getComponentIndex(componentType);
    if(componentIndex >= OtherTestEntity::componentTypeCount)
    {
        ASSERT(componentIndex < OtherTestEntity::componentTypeCount);
        return *this;
    }
    componentIndexArray |= u64(1) << u64(componentIndex);
    return *this;
}

OtherTestEntity::~OtherTestEntity()
{
    syncReadWrites();
}

bool OtherTestEntity::hasComponent(EntitySystemHandle handle, ComponentType componentType) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[handle.entityIndex] >> componentIndex) & 1) == 1;
}

bool OtherTestEntity::hasComponents(EntitySystemHandle handle,
    const OtherTestEntityComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[handle.entityIndex] & compIndArr) == compIndArr;
}

bool OtherTestEntity::hasComponent(u32 entityIndex, ComponentType componentType) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[entityIndex] >> componentIndex) & 1) == 1;
}

bool OtherTestEntity::hasComponents(u32 entityIndex,
    const OtherTestEntityComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

bool OtherTestEntity::hasComponents(u32 entityIndex, const EntityRWHandle &rwHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    if(rwHandle.rwHandleTypeId != entitySystemID || rwHandle.syncIndexPoint != currentSyncIndex)
    {
        ASSERT(rwHandle.rwHandleTypeId == entitySystemID);
        ASSERT(rwHandle.syncIndexPoint == currentSyncIndex);
        return false;
    }

    u64 compIndArr = rwHandle.readArrays | rwHandle.writeArrays;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

u32 OtherTestEntity::getComponentIndex(ComponentType componentType)
{
    // Could be written with switch-cases if it comes to that. Probably no need to though
    for(u32 i = 0; i < componentTypeCount; ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
}

const EntityRWHandle OtherTestEntity::getRWHandle(
    const OtherTestEntityComponentArrayHandleBuilder& readBuilder,
    const OtherTestEntityComponentArrayHandleBuilder& writeBuilder)
{
    u64 reads = readArrays.fetch_or(readBuilder.componentIndexArray);
    u64 writes = writeArrays.fetch_or(writeBuilder.componentIndexArray);

    reads |= readBuilder.componentIndexArray;
    writes |= writeBuilder.componentIndexArray;

    ASSERT((writes & reads) == 0);
    if((writes & reads) == 0)
    {
        return EntityRWHandle{
            .readArrays = readBuilder.componentIndexArray,
            .writeArrays = writeBuilder.componentIndexArray,
            .syncIndexPoint = currentSyncIndex,
            .rwHandleTypeId = entitySystemID
       };
    }
    return EntityRWHandle{};
}

EntitySystemHandle OtherTestEntity::getEntitySystemHandle(u32 index) const
{
    if(index >= entityComponents.size())
        return EntitySystemHandle();

    return EntitySystemHandle {
        .entitySystemType = entitySystemID,
        .entityIndexVersion = entityVersions[index],
        .entityIndex = index };
}

OtherTestEntity::OtherTestEntityEntityLockedMutexHandle OtherTestEntity::getLockedMutexHandle()
{
    entityAddRemoveMutex.lock();
    ++mutexLockIndex;
    return OtherTestEntity::OtherTestEntityEntityLockedMutexHandle { .lockIndex = mutexLockIndex };
}

bool OtherTestEntity::releaseLockedMutexHandle(const OtherTestEntity::OtherTestEntityEntityLockedMutexHandle& handle)
{
    if(mutexLockIndex != handle.lockIndex)
        return false;

    ++mutexLockIndex;
    entityAddRemoveMutex.unlock();
    return true;
}

bool OtherTestEntity::syncReadWrites()
{
    u64 reads = readArrays.load();
    u64 writes = writeArrays.load();

    readArrays.store(u64(0));
    writeArrays.store(u64(0));
    ++currentSyncIndex;

    // Cannot have both reading and writing to same array in same sync point.
    ASSERT((reads & writes) == 0);

    bool readWrite = (reads | writes) != 0;
    bool addRemove = entitiesAdded || entitiesRemoved;
    ASSERT(!(readWrite && addRemove));

    entitiesAdded = false;
    entitiesRemoved = false;

    return ((reads & writes) == 0) && !(readWrite && addRemove);
}

EntitySystemHandle OtherTestEntity::addEntity(const OtherTestEntityEntityLockedMutexHandle& handle)
{
    u32 addIndex = ~0u;

    if(freeEntityIndices.size() == 0)
    {
        Heritaged1Array.emplace_back();
        entityComponents.emplace_back(0);
        entityVersions.emplace_back(1);
        addIndex = entityComponents.size() - 1;

    }
    else
    {
        u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
        freeEntityIndices.resize(freeEntityIndices.size() - 1);
        entityComponents[freeIndex] = 0;
        ++entityVersions[freeIndex];
        addIndex = freeIndex;
    }
    entitiesAdded = true;
    return getEntitySystemHandle(addIndex);
}

bool OtherTestEntity::removeEntity(EntitySystemHandle handle, const OtherTestEntityEntityLockedMutexHandle &mutexHandle)
{
    ASSERT(mutexHandle.lockIndex == mutexLockIndex);
    if(mutexHandle.lockIndex != mutexLockIndex)
        return false;

    ASSERT(handle.entitySystemType == entitySystemID);
    if(handle.entitySystemType != entitySystemID)
        return false;

    ASSERT(handle.entityIndex < entityComponents.size());
    if(handle.entityIndex >= entityComponents.size())
        return false;

    ASSERT(handle.entityIndexVersion == entityVersions[handle.entityIndex]);
    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u32 freeIndex = handle.entityIndex;
    entityComponents[freeIndex] = 0;
    ++entityVersions[freeIndex];
    freeEntityIndices.emplace_back(freeIndex);

    entitiesRemoved = true;

    return true;
}

const Heritaged1* OtherTestEntity::getHeritaged1ReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
Heritaged1* OtherTestEntity::getHeritaged1WriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.rwHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.rwHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> componentIndex) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}

bool OtherTestEntity::addHeritaged1Component(EntitySystemHandle handle, const Heritaged1& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(Heritaged1::componentID);

    if(componentIndex >= componentTypeCount)
        return false;

    if(hasComponent(handle, Heritaged1::componentID))
        return false;

    Heritaged1Array[handle.entityIndex] = component;
    entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

    return true;
}

bool OtherTestEntity::serialize(WriteJson &json) const
{
    u32 entityAmount = entityComponents.size();
    if(entityAmount == 0)
        return false;

    json.addObject(entitySystemName);
    json.addString("EntityType", entitySystemName);
    json.addInteger("EntityTypeId", u32(entitySystemID));
    json.addInteger("EntityVersion", entityVersion);
    json.addArray("Entities");
    for(u32 i = 0; i < entityAmount; ++i)
    {
        json.addObject();
        json.addArray("Components");

            if(hasComponent(getEntitySystemHandle(i), Heritaged1::componentID))
            {
                Heritaged1Array[i].serialize(json);
            }
        json.endArray();
        json.endObject();
    }
    json.endArray();
    json.endObject();
    return json.isValid();
}

bool OtherTestEntity::deserialize(const JsonBlock &json, const OtherTestEntityEntityLockedMutexHandle &mutexHandle)
{
    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    const JsonBlock& child = json.getChild(entitySystemName);
    if(!child.isValid())
        return false;

    if(!child.getChild("EntityTypeId").equals(u32(entitySystemID)) || !child.getChild("EntityType").equals(entitySystemName))
        return false;

    u32 addedCount = 0u;
    for(const auto &entityJson : child.getChild("Entities"))
    {
        addEntity(mutexHandle);
        for(const auto &obj : entityJson.getChild("Components"))
        {
                if(Heritaged1Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
        }
        addedCount++;
    }
    return true;
}

