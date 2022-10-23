#pragma once
// This is generated file, do not modify.

#include "src/components_macro.h"
#include <math/vector3.h>
#include <vector>

struct Heritaged31
{
    static constexpr const char* componentName = "Heritaged31";
    static constexpr ComponentType componentID = ComponentType::HeritagedType;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 5;

    // Row 0, Size 4
    int TempInt = 10;
    // Row 1, Size 4
    float TempFloat = 20.0f;
    // Row 2, Size 8
    Vector2 TempV2 = {41, 520};
    // Row 3, Size 12
    Vector3 TempV3 = {10, 20, 30};
    // Row 4, Size 16
    Vector4 TempV4 = {7, 8, -30, 10};

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
        "TempInt",
        "TempFloat",
        "TempV2",
        "TempV3",
        "TempV4",
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
            case 0: return &TempInt;
            case 1: return &TempFloat;
            case 2: return &TempV2;
            case 3: return &TempV3;
            case 4: return &TempV4;
            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }

    const void* getElementIndex(u32 index) const
    {
        switch(index)
        {
            case 0: return &TempInt;
            case 1: return &TempFloat;
            case 2: return &TempV2;
            case 3: return &TempV3;
            case 4: return &TempV4;
            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }
};

struct Heritaged21
{
    static constexpr const char* componentName = "Heritaged21";
    static constexpr ComponentType componentID = ComponentType::HeritagedType2;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 2;

    // Row 0, Size 4
    int TempInt2 = 10;
    // Row 1, Size 4
    float TempFloat2 = 20.0f;

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
    };
    static constexpr const char* fieldNames[] =
    {
        "TempInt2",
        "TempFloat2",
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
            case 0: return &TempInt2;
            case 1: return &TempFloat2;
            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }

    const void* getElementIndex(u32 index) const
    {
        switch(index)
        {
            case 0: return &TempInt2;
            case 1: return &TempFloat2;
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
        Heritaged31::componentID,
        Heritaged21::componentID,
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

    bool hasComponent(u32 entityIndex, ComponentType componentType) const
    {
        if(entityIndex >= entityComponents.size())
            return false;
        u64 componentIndex = getComponentIndex(componentType);

        if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
            return false;

        return ((entityComponents[entityIndex] >> componentIndex) & 1) == 1;
    }

    EntitySystemHandle getEntitySystemHandle(u32 index) const
    {
        if(index >= entityComponents.size())
            return EntitySystemHandle();

        return EntitySystemHandle {
            .entitySystemIndex = entitySystemID,
            .entityIndexVersion = entityVersions[index],
            .entityIndex = index };
    }
    EntitySystemHandle addEntity()
    {
        if(freeEntityIndices.size() == 0)
        {
            Her31Array.emplace_back();
            Her21Array.emplace_back();
            entityComponents.emplace_back(0);
            entityVersions.emplace_back(0);
            return getEntitySystemHandle(entityComponents.size() - 1);
        }
        else
        {
            u32 freeIndex = freeEntityIndices[freeEntityIndices.size() - 1];
            freeEntityIndices.resize(freeEntityIndices.size() - 1);
            entityComponents[freeIndex] = 0;
            ++entityVersions[freeIndex];
            return getEntitySystemHandle(freeIndex);
        }
        return EntitySystemHandle();
    }

    bool addHeritaged31Component(u32 entityIndex, const Heritaged31& component)
    {
        if(entityIndex >= entityComponents.size())
            return false;

        u64 componentIndex = getComponentIndex(Heritaged31::componentID);

        if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
            return false;

        if(hasComponent(entityIndex, Heritaged31::componentID))
            return false;

        Her31Array[entityIndex] = component;
        entityComponents[entityIndex] |= u64(1) << componentIndex;

        return true;
    }

    bool addHeritaged21Component(u32 entityIndex, const Heritaged21& component)
    {
        if(entityIndex >= entityComponents.size())
            return false;

        u64 componentIndex = getComponentIndex(Heritaged21::componentID);

        if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
            return false;

        if(hasComponent(entityIndex, Heritaged21::componentID))
            return false;

        Her21Array[entityIndex] = component;
        entityComponents[entityIndex] |= u64(1) << componentIndex;

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

            if(hasComponent(i, Heritaged31::componentID))
            {
                Her31Array[i].serialize(json);
            }
            if(hasComponent(i, Heritaged21::componentID))
            {
                Her21Array[i].serialize(json);
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
                if(Her31Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged31::componentID);
                    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }
                if(Her21Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(Heritaged21::componentID);
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
    std::vector<Heritaged31> Her31Array;
    std::vector<Heritaged21> Her21Array;

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(sizeof(componentTypes) / sizeof(ComponentType) < 64, "Only 64 components are allowed for entity!");
};
