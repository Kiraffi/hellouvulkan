#function(PARSE_DEF_FILE SOURCE_DEF_FILE DES_DEF_FILE)

# https://www.reddit.com/r/cmake/comments/iokem9/force_cmake_rerun_when_file_changes/
# set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SOURCE_DEF_FILE}")

set(READ_STATE_NONE 0)
set(READ_STATE_COMPONENT_BEGIN 1)
set(READ_STATE_COMPONENT_FIELDS 2)
#set(READ_STATE_COMPONENT_END 3)
set(READ_STATE_ENTITY_BEGIN 10)
set(READ_STATE_ENTITY_FIELDS 11)
#set(READ_STATE_ENTITY_END 12)

set(COMPONENT_NAME "")
set(COMPONENT_ID -1)
set(COMPONENT_VERSION -1)

set(ENTITY_NAME "")
set(ENTITY_ID 0)
set(ENTITY_VERSION 0)

# Set our reading state to 0, there is probably a lot better way
set(READ_STATE ${READ_STATE_NONE})
set(COMPONENT_FIELD_COUNT 0)
set(FILENAME_TO_MODIFY "${DES_DEF_FILE}")
# Read the entire DEF file.
file(READ "${SOURCE_DEF_FILE}" DEF_CONTENTS)

# Split the DEF-file by new-lines.
string(REPLACE "\n" ";" DEF_LIST ${DEF_CONTENTS})

#not sure if this write + append is good....
file(WRITE ${FILENAME_TO_MODIFY}
"#pragma once
// This is generated file, do not modify.

#include \"src/components.h\"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <vector>\n")

# Loop through each line in the DEF file.
foreach(DEF_ROW ${DEF_LIST})
    #remove trailing and leading spaces
    string(STRIP "${DEF_ROW}" DEF_ROW)


################################### PARSE ENTITY ############################################

    if(DEF_ROW STREQUAL "EntityTypeEnd" AND READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

        file(APPEND ${FILENAME_TO_MODIFY} "
struct ${ENTITY_NAME}
{
    static constexpr const char* entitySystemName = \"${ENTITY_NAME}\";
    static constexpr EntityType entitySystemID = ${ENTITY_ID};
    static constexpr u32 entityVersion = ${ENTITY_VERSION};

    static constexpr ComponentType componentTypes[] =
    {${ENTITY_COMPONENT_TYPES_ARRAY}
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
        {${ENTITY_ARRAY_PUSHBACKS}
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
${ENTITY_ADD_COMPONENT}
    bool serialize(WriteJson &json) const
    {
        u32 entityAmount = entityComponents.size();
        if(entityAmount == 0)
            return false;

        json.addObject(entitySystemName);
        json.addString(\"EntityType\", entitySystemName);
        json.addInteger(\"EntityTypeId\", u32(entitySystemID));
        json.addInteger(\"EntityVersion\", entityVersion);
        json.addArray(\"Entities\");
        for(u32 i = 0; i < entityAmount; ++i)
        {
            json.addObject();
            json.addArray(\"Components\");
${ENTITY_WRITE_CONTENTS}
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

        if(!child.getChild(\"EntityTypeId\").equals(u32(entitySystemID)) || !child.getChild(\"EntityType\").equals(entitySystemName))
            return false;

        u32 addedCount = 0u;
        for(const auto &entityJson : child.getChild(\"Entities\"))
        {
            addEntity();
            for(const auto &obj : entityJson.getChild(\"Components\"))
            {${ENTITY_LOAD_CONTENTS}
            }
            addedCount++;
        }
        return true;
    }

private:${ENTITY_ARRAYS_FIELD}

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(sizeof(componentTypes) / sizeof(ComponentType) < 64, \"Only 64 components are allowed for entity!\");
};\n")

    elseif(DEF_ROW STREQUAL "EntityTypeBegin"  AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_BEGIN})
        set(ENTITY_LOAD_CONTENTS "")
        set(ENTITY_WRITE_CONTENTS "")
        set(ENTITY_ARRAYS_FIELD "")
        set(ENTITY_ARRAY_PUSHBACKS "")
        set(ENTITY_ADD_COMPONENT "")
        set(ENTITY_COMPONENT_TYPES_ARRAY "")

    elseif(READ_STATE EQUAL READ_STATE_ENTITY_BEGIN)
        set(READ_STATE ${READ_STATE_ENTITY_FIELDS})

        string(REPLACE " " ";" DEF_ROW_CONTENTS ${DEF_ROW})

        list(GET DEF_ROW_CONTENTS 0 ELEM0) # component name
        list(GET DEF_ROW_CONTENTS 1 ELEM1) # component id
        list(GET DEF_ROW_CONTENTS 2 ELEM2) # component version
        #remove leading and trailing spaces
        string(STRIP ${ELEM0} ELEM0)
        string(STRIP ${ELEM1} ELEM1)
        string(STRIP ${ELEM2} ELEM2)

        set(ENTITY_NAME ${ELEM0})
        set(ENTITY_ID ${ELEM1})
        set(ENTITY_VERSION ${ELEM2})




    elseif(READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)

        # Get a list of the elements in this DEF row.
        string(REPLACE "?" ";" TYPE_AND_NAME "${DEF_ROW}")

        list(GET TYPE_AND_NAME 0 ELEM1) # type
        list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        #remove leading and trailing spaces
        string(STRIP "${ELEM0}" ELEM0)
        string(STRIP "${ELEM1}" ELEM1)

        string(APPEND ENTITY_WRITE_CONTENTS "
            if(hasComponent(getEntitySystemHandle(i), ${ELEM0}::componentID))
            {
                ${ELEM1}Array[i].serialize(json);
            }")
        string(APPEND ENTITY_COMPONENT_TYPES_ARRAY "\n        ${ELEM0}::componentID,")
        string(APPEND ENTITY_ARRAY_PUSHBACKS "\n            ${ELEM1}Array.emplace_back();")
        string(APPEND ENTITY_ARRAYS_FIELD "\n    std::vector<${ELEM0}> ${ELEM1}Array;")
        string(APPEND ENTITY_LOAD_CONTENTS "
                if(${ELEM1}Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
                    if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }")

        string(APPEND ENTITY_ADD_COMPONENT "
    bool add${ELEM0}Component(EntitySystemHandle handle, const ${ELEM0}& component)
    {
        // Some error if no lock

        if(handle.entitySystemType != entitySystemID)
            return false;

        if(handle.entityIndex >= entityComponents.size())
            return false;

        if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
            return false;

        u64 componentIndex = getComponentIndex(${ELEM0}::componentID);

        if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
            return false;

        if(hasComponent(handle, ${ELEM0}::componentID))
            return false;

        ${ELEM1}Array[handle.entityIndex] = component;
        entityComponents[handle.entityIndex] |= u64(1) << componentIndex;

        return true;
    }\n")



################################ PARSE COMPONENT #############################################



    elseif(DEF_ROW STREQUAL "ComponentEnd" AND READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

        file(APPEND ${FILENAME_TO_MODIFY} "
struct ${COMPONENT_NAME}
{
    static constexpr const char* componentName = \"${COMPONENT_NAME}\";
    static constexpr ComponentType componentID = ${COMPONENT_ID};
    static constexpr u32 componentVersion = ${COMPONENT_VERSION};
    static constexpr u32 componentFieldAmount = ${COMPONENT_FIELD_COUNT};
${COMPONENT_VARS}

    static constexpr FieldType fieldTypes[] =
    {${COMPONENT_FIELD_TYPES}
    };
    static constexpr const char* fieldNames[] =
    {${COMPONENT_FIELD_NAMES}
    };

    bool serialize(WriteJson &json) const
    {
        json.addObject();
        json.addString(\"ComponentType\", componentName);
        json.addInteger(\"ComponentTypeId\", u32(componentID));
        json.addInteger(\"ComponentVersion\", componentVersion);

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

        if(!json.getChild(\"ComponentType\").equals(componentName))
            return false;
        if(!json.getChild(\"ComponentTypeId\").equals(u32(componentID)))
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
        {${COMPONENT_ELEMENT_GET_FUNC}
            default: ASSERT_STRING(false, \"Unknown index\");
        }
        return nullptr;
    }

    const void* getElementIndex(u32 index) const
    {
        switch(index)
        {${COMPONENT_ELEMENT_GET_FUNC}
            default: ASSERT_STRING(false, \"Unknown index\");
        }
        return nullptr;
    }
};\n")

    elseif(DEF_ROW STREQUAL "ComponentBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_COMPONENT_BEGIN})
        set(COMPONENT_FIELD_COUNT 0)
        set(COMPONENT_VARS "")
        set(COMPONENT_FIELD_TYPES "")
        set(COMPONENT_FIELD_NAMES "")
        set(COMPONENT_ELEMENT_GET_FUNC "")

    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_BEGIN)
        set(READ_STATE "${READ_STATE_COMPONENT_FIELDS}")

        string(REPLACE " " ";" DEF_ROW_CONTENTS "${DEF_ROW}")

        list(GET DEF_ROW_CONTENTS 0 ELEM0) # component name
        list(GET DEF_ROW_CONTENTS 1 ELEM1) # component id
        list(GET DEF_ROW_CONTENTS 2 ELEM2) # component version
        #remove leading and trailing spaces
        string(STRIP "${ELEM0}" ELEM0)
        string(STRIP "${ELEM1}" ELEM1)
        string(STRIP "${ELEM2}" ELEM2)

        set(COMPONENT_NAME ${ELEM0})
        set(COMPONENT_ID ${ELEM1})
        set(COMPONENT_VERSION ${ELEM2})

    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)

        # Get a list of the elements in this DEF row.
        string(REPLACE "=" ";" LEFT_RIGHT_EQ ${DEF_ROW})
        list(GET LEFT_RIGHT_EQ 1 ELEM2) # default value
        list(GET LEFT_RIGHT_EQ 0 TYPE_AND_NAME) # type and name

        string(REPLACE "?" ";" TYPE_AND_NAME ${TYPE_AND_NAME})
        list(GET TYPE_AND_NAME 0 ELEM1) # type
        list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        #remove leading and trailing spaces
        string(STRIP "${ELEM0}" ELEM0)
        string(STRIP "${ELEM1}" ELEM1)
        string(STRIP "${ELEM2}" ELEM2)

        set(COMPONENT_FIELD_ELEMENT_SIZE "sizeof(${ELEM0})")
        if(ELEM0 STREQUAL "int")
            set(COMPONENT_FIELD_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::IntType,")
        elseif(ELEM0 STREQUAL "float")
            set(COMPONENT_FIELD_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::FloatType,")
        elseif(ELEM0 STREQUAL "Vector2")
            set(COMPONENT_FIELD_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec2Type,")
        elseif(ELEM0 STREQUAL "Vector3")
            set(COMPONENT_FIELD_ELEMENT_SIZE 12)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec3Type,")
        elseif(ELEM0 STREQUAL "Vector4")
            set(COMPONENT_FIELD_ELEMENT_SIZE 16)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec4Type,")
        endif()

        string(APPEND COMPONENT_ELEMENT_GET_FUNC "
            case ${COMPONENT_FIELD_COUNT}: return &${ELEM1};")
        string(APPEND COMPONENT_FIELD_NAMES "\n        \"${ELEM1}\",")

        string(APPEND COMPONENT_VARS "
    // Row ${COMPONENT_FIELD_COUNT}, Size ${COMPONENT_FIELD_ELEMENT_SIZE}
    ${ELEM0} ${ELEM1} = ${ELEM2};")

        # Count up the COMPONENT_FIELD_COUNT, there is probably far better way for adding...
        math(EXPR COMPONENT_FIELD_COUNT "${COMPONENT_FIELD_COUNT} + 1" OUTPUT_FORMAT DECIMAL)

    endif()
endforeach()
#endfunction(PARSE_DEF_FILE)
