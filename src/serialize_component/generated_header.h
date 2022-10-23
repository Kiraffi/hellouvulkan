#pragma once
// This is generated file, do not modify.

#include "src/components.h"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <vector>

struct Heritaged1
{
    static constexpr const char* componentName = "Heritaged1";
    static constexpr ComponentType componentID = ComponentType::HeritagedType;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 5;

    // Row 0, Size 4
    int tempInt = 10;
    // Row 1, Size 4
    float tempFloat = 20.0f;
    // Row 2, Size 8
    Vector2 tempV2 = {41, 520};
    // Row 3, Size 12
    Vector3 tempV3 = {10, 20, 30};
    // Row 4, Size 16
    Vector4 tempV4 = {7, 8, -30, 10};

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
        FieldType::Vec2Type,
        FieldType::Vec3Type,
        FieldType::Vec4Type,
    };
    static constexpr const char* fieldNames[] =
    {
        "tempInt",
        "tempFloat",
        "tempV2",
        "tempV3",
        "tempV4",
    };

    bool serialize(WriteJson &json) const
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

    bool deserialize(const JsonBlock &json)
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

private:
    void* getElementIndexRef(u32 index)
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

    const void* getElementIndex(u32 index) const
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
};

struct Heritaged2
{
    static constexpr const char* componentName = "Heritaged2";
    static constexpr ComponentType componentID = ComponentType::HeritagedType2;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 2;

    // Row 0, Size 4
    int tempInt2 = 10;
    // Row 1, Size 4
    float tempFloat2 = 20.0f;

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
    };
    static constexpr const char* fieldNames[] =
    {
        "tempInt2",
        "tempFloat2",
    };

    bool serialize(WriteJson &json) const
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

    bool deserialize(const JsonBlock &json)
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

private:
    void* getElementIndexRef(u32 index)
    {
        switch(index)
        {
            case 0: return &tempInt2;
            case 1: return &tempFloat2;
            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }

    const void* getElementIndex(u32 index) const
    {
        switch(index)
        {
            case 0: return &tempInt2;
            case 1: return &tempFloat2;
            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }
};

struct StaticModelEntity
{
    static constexpr const char* entitySystemName = "StaticModelEntity";
    static constexpr EntityType entitySystemID = EntityType::StaticModelEntityType;
    static constexpr u32 entityVersion = 1;

    static constexpr ComponentType componentTypes[] =
    {
        Heritaged1::componentID,
        Heritaged2::componentID,
    };

    u32 getComponentIndex(ComponentType componentType) const
    {
        for(u32 i = 0; i < sizeof(componentTypes) / sizeof(ComponentType); ++i)
        {
            if(componentType == componentTypes[i])
                return i;
        }
        return ~0u;
    }

    bool hasComponent(EntitySystemHandle handle, ComponentType componentType) const
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

    EntitySystemHandle getEntitySystemHandle(u32 index) const
    {
        if(index >= entityComponents.size())
            return EntitySystemHandle();

        return EntitySystemHandle {
            .entitySystemType = entitySystemID,
            .entityIndexVersion = entityVersions[index],
            .entityIndex = index };
    }

    EntitySystemHandle addEntity()
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

    bool removeEntity(EntitySystemHandle handle)
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

    bool addHeritaged1Component(EntitySystemHandle handle, const Heritaged1& component)
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

    bool addHeritaged2Component(EntitySystemHandle handle, const Heritaged2& component)
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

    bool serialize(WriteJson &json) const
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

    bool deserialize(const JsonBlock &json)
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

private:
    std::vector<Heritaged1> her1Array;
    std::vector<Heritaged2> her2Array;

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(sizeof(componentTypes) / sizeof(ComponentType) < 64, "Only 64 components are allowed for entity!");
};
