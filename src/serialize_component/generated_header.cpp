// This is generated file, do not modify.
#include "generated_header.h"

#include "src/components.h"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

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
            case 2: return &tempFloat2;
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
            case 2: return &tempFloat2;
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

    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
        return false;

    return ((entityComponents[handle.entityIndex] >> componentIndex) & 1) == 1;
}

u32 StaticModelEntity::getComponentIndex(ComponentType componentType) const
{
    for(u32 i = 0; i < sizeof(componentTypes) / sizeof(ComponentType); ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
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
            her1Array.emplace_back();
            her2Array.emplace_back();
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

const Heritaged1* StaticModelEntity::gether1ReadArray() const
{
    return her1Array.data();
}
Heritaged1* StaticModelEntity::gether1WriteArray()
{
    return her1Array.data();
}
const Heritaged2* StaticModelEntity::gether2ReadArray() const
{
    return her2Array.data();
}
Heritaged2* StaticModelEntity::gether2WriteArray()
{
    return her2Array.data();
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

    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
        return false;

    if(hasComponent(handle, Heritaged1::componentID))
        return false;

    her1Array[handle.entityIndex] = component;
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

    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
        return false;

    if(hasComponent(handle, Heritaged2::componentID))
        return false;

    her2Array[handle.entityIndex] = component;
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
                her1Array[i].serialize(json);
            }
            if(hasComponent(getEntitySystemHandle(i), Heritaged2::componentID))
            {
                her2Array[i].serialize(json);
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
                if(her1Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
                    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
                if(her2Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged2::componentID);
                    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
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

    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
        return false;

    return ((entityComponents[handle.entityIndex] >> componentIndex) & 1) == 1;
}

u32 OtherTestEntity::getComponentIndex(ComponentType componentType) const
{
    for(u32 i = 0; i < sizeof(componentTypes) / sizeof(ComponentType); ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
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
            her1Array.emplace_back();
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

const Heritaged1* OtherTestEntity::gether1ReadArray() const
{
    return her1Array.data();
}
Heritaged1* OtherTestEntity::gether1WriteArray()
{
    return her1Array.data();
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

    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
        return false;

    if(hasComponent(handle, Heritaged1::componentID))
        return false;

    her1Array[handle.entityIndex] = component;
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
                her1Array[i].serialize(json);
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
                if(her1Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged1::componentID);
                    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
        }
        addedCount++;
    }
    return true;
}

