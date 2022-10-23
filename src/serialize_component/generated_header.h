#pragma once
// This is generated file, do not modify.

#include "src/components_macro.h"
#include <math/vector3.h>
#include <vector>

struct Heritaged31
{
    static constexpr const char* componentName = "Heritaged31";
    static constexpr unsigned int componentID = ComponentType::HeritagedType;
    static constexpr unsigned int componentVersion = 1;
    static constexpr unsigned int componentFieldAmount = 5;

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

    void* getElementIndexRef(unsigned int index)
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

    const void* getElementIndex(unsigned int index) const
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

    bool serialize(WriteJson &json) const
    {
        json.addObject();
        json.addString("ComponentType", componentName);
        json.addInteger("ComponentTypeId", componentID);
        json.addInteger("ComponentVersion", componentVersion);

        for(int i = 0; i < componentFieldAmount; ++i)
        {
            if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
                return false;
        }
        return json.isValid();
    }

    bool deserialize(const JsonBlock &json)
    {
        if(!json.isObject() || !json.isValid())
            return false;

        if(!json.getChild("ComponentType").equals(componentName))
            return false;
        if(!json.getChild("ComponentTypeId").equals(componentID))
            return false;
        for(unsigned int i = 0; i < componentFieldAmount; ++i)
        {
            deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
        }
        return true;
    }
};

struct Heritaged21
{
    static constexpr const char* componentName = "Heritaged21";
    static constexpr unsigned int componentID = ComponentType::HeritagedType2;
    static constexpr unsigned int componentVersion = 1;
    static constexpr unsigned int componentFieldAmount = 2;

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

    void* getElementIndexRef(unsigned int index)
    {
        switch(index)
        {
            case 0: return &TempInt2;
            case 1: return &TempFloat2;

            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }

    const void* getElementIndex(unsigned int index) const
    {
        switch(index)
        {
            case 0: return &TempInt2;
            case 1: return &TempFloat2;

            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }

    bool serialize(WriteJson &json) const
    {
        json.addObject();
        json.addString("ComponentType", componentName);
        json.addInteger("ComponentTypeId", componentID);
        json.addInteger("ComponentVersion", componentVersion);

        for(int i = 0; i < componentFieldAmount; ++i)
        {
            if(!serializeField(json, fieldNames[i], getElementIndex(i), fieldTypes[i]))
                return false;
        }
        return json.isValid();
    }

    bool deserialize(const JsonBlock &json)
    {
        if(!json.isObject() || !json.isValid())
            return false;

        if(!json.getChild("ComponentType").equals(componentName))
            return false;
        if(!json.getChild("ComponentTypeId").equals(componentID))
            return false;
        for(unsigned int i = 0; i < componentFieldAmount; ++i)
        {
            deserializeField(json, fieldNames[i], getElementIndexRef(i), fieldTypes[i]);
        }
        return true;
    }
};


struct StaticModelEntity
{
    static constexpr const char* entityName = "StaticModelEntity";
    static constexpr unsigned int entityID = EntityType::StaticModelEntityType;
    static constexpr unsigned int entityVersion = 1;
    bool serialize() const
    {
        return true;
    }
    bool deserialize(const JsonBlock &json)
    {
        if(!json.isObject() || json.getChildCount() < 1)
            return false;

        unsigned int addedCount = 0u;
        for(const JsonBlock& child : json)
        {
            if(!child.getChild("EntityID").equals(entityID) || !child.getChild("EntityType").equals(entityName))
                return false;

            for(auto const &obj : child.getChild("Components"))
            {
                if(Her31Array[addedCount].deserialize(obj))
                {
                    continue;
                }
                if(Her21Array[addedCount].deserialize(obj))
                {
                    continue;
                }
            }
        }
        addedCount++;
        return true;
    }
private:
    std::vector<Heritaged31> Her31Array;
    std::vector<Heritaged21> Her21Array;
    std::vector<u64> entityComponents;
};
