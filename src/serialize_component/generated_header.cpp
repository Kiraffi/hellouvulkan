// This is generated file, do not modify.
#include "generated_header.h"

#include "src/components.h"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <atomic>
#include <vector>

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
        default: ASSERT_STRING(false, "Unknown index");
    }
    return nullptr;
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

const EntityReadWriteHandle StaticModelEntity::getReadWriteHandle(const StaticModelEntityReadWriteHandleBuilder& builder)
{
    u64 writes = writeArrays.fetch_or(builder.writeArrays);
    u64 reads = readArrays.fetch_or(builder.readArrays);

    reads |= builder.readArrays;
    writes |= builder.writeArrays;

    ASSERT((writes & reads) == 0);
    if((writes & reads) == 0)
    {
        return EntityReadWriteHandle{ 
            .readArrays = builder.readArrays,
            .writeArrays = builder.writeArrays,
            .syncIndexPoint = currentSyncIndex,
            .readWriteHandleTypeId = entitySystemID
       };
    }
    return EntityReadWriteHandle{};
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

EntitySystemHandle StaticModelEntity::addEntity()
{
    // Some error if no lock

    if(freeEntityIndices.size() == 0)
    {
            Heritaged1Array.emplace_back();
            Heritaged2Array.emplace_back();
        entityComponents.emplace_back(0);
        entityVersions.emplace_back(0);
        return getEntitySystemHandle(entityComponents.size() - 1);
    }
    else
    {
        u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
        freeEntityIndices.resize(freeEntityIndices.size() - 1);
        entityComponents[freeIndex] = 0;
        return getEntitySystemHandle(freeIndex);
    }
    return EntitySystemHandle();
}

bool StaticModelEntity::removeEntity(EntitySystemHandle handle)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u32 freeIndex = handle.entityIndex;
    entityComponents[freeIndex] = 0;
    ++entityVersions[freeIndex];
    freeEntityIndices.emplace_back(freeIndex);
    return true;
}

const Heritaged1* StaticModelEntity::getHeritaged1ReadArray(const EntityReadWriteHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> u64(Heritaged1::componentID)) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
Heritaged1* StaticModelEntity::getHeritaged1WriteArray(const EntityReadWriteHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> u64(Heritaged1::componentID)) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
const Heritaged2* StaticModelEntity::getHeritaged2ReadArray(const EntityReadWriteHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> u64(Heritaged2::componentID)) & 1) == 1);
        return nullptr;
    }
    return Heritaged2Array.data();
}
Heritaged2* StaticModelEntity::getHeritaged2WriteArray(const EntityReadWriteHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> u64(Heritaged2::componentID)) & 1) == 1);
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

bool StaticModelEntity::deserialize(const JsonBlock &json)
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
        addEntity();
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

const EntityReadWriteHandle OtherTestEntity::getReadWriteHandle(const OtherTestEntityReadWriteHandleBuilder& builder)
{
    u64 writes = writeArrays.fetch_or(builder.writeArrays);
    u64 reads = readArrays.fetch_or(builder.readArrays);

    reads |= builder.readArrays;
    writes |= builder.writeArrays;

    ASSERT((writes & reads) == 0);
    if((writes & reads) == 0)
    {
        return EntityReadWriteHandle{ 
            .readArrays = builder.readArrays,
            .writeArrays = builder.writeArrays,
            .syncIndexPoint = currentSyncIndex,
            .readWriteHandleTypeId = entitySystemID
       };
    }
    return EntityReadWriteHandle{};
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

EntitySystemHandle OtherTestEntity::addEntity()
{
    // Some error if no lock

    if(freeEntityIndices.size() == 0)
    {
            Heritaged1Array.emplace_back();
        entityComponents.emplace_back(0);
        entityVersions.emplace_back(0);
        return getEntitySystemHandle(entityComponents.size() - 1);
    }
    else
    {
        u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
        freeEntityIndices.resize(freeEntityIndices.size() - 1);
        entityComponents[freeIndex] = 0;
        return getEntitySystemHandle(freeIndex);
    }
    return EntitySystemHandle();
}

bool OtherTestEntity::removeEntity(EntitySystemHandle handle)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u32 freeIndex = handle.entityIndex;
    entityComponents[freeIndex] = 0;
    ++entityVersions[freeIndex];
    freeEntityIndices.emplace_back(freeIndex);
    return true;
}

const Heritaged1* OtherTestEntity::getHeritaged1ReadArray(const EntityReadWriteHandle& handle) const
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> u64(Heritaged1::componentID)) & 1) == 1);
        return nullptr;
    }
    return Heritaged1Array.data();
}
Heritaged1* OtherTestEntity::getHeritaged1WriteArray(const EntityReadWriteHandle& handle)
{
    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> u64(Heritaged1::componentID)) & 1) == 1);
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

bool OtherTestEntity::deserialize(const JsonBlock &json)
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
        addEntity();
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

