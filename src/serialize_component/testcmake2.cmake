function(PARSE_DEF_FILE FROM_FILE TO_FILE)

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

# Set our reading state to 0, there is probably a lot better way
set(READ_STATE ${READ_STATE_NONE})
set(COMPONENT_FIELD_COUNT 0)
set(FILENAME_TO_MODIFY "${TO_FILE}")
# Read the entire DEF file.
file(READ "${FROM_FILE}" DEF_CONTENTS)

# Split the DEF-file by new-lines.
string(REPLACE "\n" ";" DEF_LIST ${DEF_CONTENTS})

set(COMPONENT_DEF_STR "")

#not sure if this write + append is good....
file(WRITE ${FILENAME_TO_MODIFY}
"#pragma once
// This is generated file, do not modify.

#include \"src/components_macro.h\"
#include <math/vector3.h>
#include <vector>\n")

# Loop through each line in the DEF file.
foreach(DEF_ROW ${DEF_LIST})
    #remove trailing and leading spaces
    string(STRIP ${DEF_ROW} DEF_ROW)


################################### PARSE ENTITY ############################################

    if(DEF_ROW STREQUAL "EntityTypeEnd" AND READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

        string(APPEND ENTITY_COMPONENT_TYPES_ARRAY "\n    };")

        file(APPEND ${FILENAME_TO_MODIFY} "\n${ENTITY_CONTENTS_BEGIN}")
        file(APPEND ${FILENAME_TO_MODIFY} "\n\n${ENTITY_COMPONENT_TYPES_ARRAY}")
        file(APPEND ${FILENAME_TO_MODIFY} "\n
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
            ++entityVersions[freeIndex];
            return getEntitySystemHandle(freeIndex);
        }
        return EntitySystemHandle();
    }
${ENTITY_ADD_COMPONENT}
    bool serialize(WriteJson &json) const
    {
        u32 entityAmount = entityComponents.size();
        if(entityAmount == 0)
            return false;

        json.addArray(entitySystemName);
        for(u32 i = 0; i < entityAmount; ++i)
        {
            json.addObject();
            json.addString(\"EntityType\", entitySystemName);
            json.addInteger(\"EntityTypeId\", u32(entitySystemID));
            json.addInteger(\"EntityVersion\", entityVersion);
            json.addArray(\"Components\");
${ENTITY_WRITE_CONTENTS}
            json.endArray();
            json.endObject();
        }
        json.endArray();
        return json.isValid();
    }
    bool deserialize(const JsonBlock &json)
    {
        if(!json.isObject() || json.getChildCount() < 1)
            return false;

        u32 addedCount = 0u;
        for(const JsonBlock& child : json.getChild(entitySystemName))
        {
            if(!child.getChild(\"EntityTypeId\").equals(u32(entitySystemID)) || !child.getChild(\"EntityType\").equals(entitySystemName))
                return false;

            addEntity();
            for(auto const &obj : child.getChild(\"Components\"))
            {
${ENTITY_LOAD_CONTENTS}
            }
        }
        addedCount++;
        return true;
    }
${ENTITY_ARRAYS_FIELD}
\n
    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(sizeof(componentTypes) / sizeof(ComponentType) < 64, \"Only 64 components are allowed for entity!\");
};\n")

    elseif(DEF_ROW STREQUAL "EntityTypeBegin"  AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_BEGIN})
        set(ENTITY_CONTENTS_BEGIN "")
        set(ENTITY_LOAD_CONTENTS "")
        set(ENTITY_WRITE_CONTENTS "")
        set(ENTITY_ARRAYS_FIELD "\n\nprivate:")
        set(ENTITY_ARRAY_PUSHBACKS "")
        set(ENTITY_ADD_COMPONENT "")
        set(ENTITY_COMPONENT_TYPES_ARRAY "    static constexpr ComponentType componentTypes[] = \n    {")

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
        set(ENTITY_CONTENTS_BEGIN "
struct ${ENTITY_NAME}
{
    static constexpr const char* entitySystemName = \"${ENTITY_NAME}\";
    static constexpr EntityType entitySystemID = ${ELEM1};
    static constexpr u32 entityVersion = ${ELEM2};")




    elseif(READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)

        # Get a list of the elements in this DEF row.
        string(REPLACE "?" ";" TYPE_AND_NAME "${DEF_ROW}")

        list(GET TYPE_AND_NAME 0 ELEM1) # type
        list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        #remove leading and trailing spaces
        string(STRIP ${ELEM0} ELEM0)
        string(STRIP ${ELEM1} ELEM1)

        string(APPEND ENTITY_WRITE_CONTENTS "
            if(hasComponent(i, ${ELEM0}::componentID))
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

        string(APPEND ENTITY_ADD_COMPONENT "\n
    bool add${ELEM0}Component(u32 entityIndex, const ${ELEM0}& component)
    {
        if(entityIndex >= entityComponents.size())
            return false;

        u64 componentIndex = getComponentIndex(${ELEM0}::componentID);

        if(componentIndex >= sizeof(componentTypes) / sizeof(ComponentType))
            return false;

        if(hasComponent(entityIndex, ${ELEM0}::componentID))
            return false;

        ${ELEM1}Array[entityIndex] = component;
        entityComponents[entityIndex] |= u64(1) << componentIndex;

        return true;
    }")



################################ PARSE COMPONENT #############################################



    elseif(DEF_ROW STREQUAL "ComponentEnd" AND READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})
        string(APPEND COMPONENT_FIELD_TYPES "\n    };")
        string(APPEND COMPONENT_FIELD_NAMES "\n    };")
        #string(APPEND COMPONENT_FIELD_OFFSETS "\n    };")
        string(APPEND COMPONENT_STATIC_VARS "    static constexpr u32 componentFieldAmount = ${COMPONENT_FIELD_COUNT};")
        string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n
            default: ASSERT_STRING(false, \"Unknown index\");
        }
        return nullptr;")

        string(APPEND COMPONENT_DEF_STR "\n${COMPONENT_STATIC_VARS}
${COMPONENT_VARS}\n
${COMPONENT_FIELD_TYPES}\n
${COMPONENT_FIELD_NAMES}\n
    void* getElementIndexRef(u32 index)
    {
        switch(index)
        {${COMPONENT_ELEMENT_GET_FUNC}
    }

    const void* getElementIndex(u32 index) const
    {
        switch(index)
        {${COMPONENT_ELEMENT_GET_FUNC}
    }

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
};\n")

        file(APPEND ${FILENAME_TO_MODIFY} "${COMPONENT_DEF_STR}")

    elseif(DEF_ROW STREQUAL "ComponentBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_COMPONENT_BEGIN})
        set(COMPONENT_FIELD_COUNT 0)
        set(COMPONENT_DEF_STR "\nstruct ")
        set(COMPONENT_VARS "")
        #set(COMPONENT_FIELD_OFFSETS "    constexpr size_t fieldOffsets[] =\n    {")
        set(COMPONENT_FIELD_TYPES "    static constexpr FieldType fieldTypes[] =\n    {")
        set(COMPONENT_FIELD_NAMES "    static constexpr const char* fieldNames[] =\n    {")
        set(COMPONENT_ELEMENT_GET_FUNC "")

    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_BEGIN)
        set(READ_STATE ${READ_STATE_COMPONENT_FIELDS})

        # Get a list of the elements in this DEF row.
        string(REPLACE " " ";" DEF_ROW_CONTENTS ${DEF_ROW})
        #message(STATUS "contents = ${DEF_ROW_CONTENTS}")


        #list(LENGTH DEF_ROW_CONTENTS len)
        #message(STATUS "len = ${len}")
        # len = 3

        # Get variables to each element.

        list(GET DEF_ROW_CONTENTS 0 ELEM0) # component name
        list(GET DEF_ROW_CONTENTS 1 ELEM1) # component id
        list(GET DEF_ROW_CONTENTS 2 ELEM2) # component version
        #remove leading and trailing spaces
        string(STRIP ${ELEM0} ELEM0)
        string(STRIP ${ELEM1} ELEM1)
        string(STRIP ${ELEM2} ELEM2)

        string(APPEND COMPONENT_DEF_STR "${ELEM0}\n{")
        set(COMPONENT_NAME ${ELEM0})
        set(COMPONENT_ID ${ELEM1})
        set(COMPONENT_VERSION ${ELEM2})

        set(COMPONENT_STATIC_VARS
"    static constexpr const char* componentName = \"${ELEM0}\";
    static constexpr ComponentType componentID = ${ELEM1};
    static constexpr u32 componentVersion = ${ELEM2};\n")

    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)

        # Get a list of the elements in this DEF row.
        string(REPLACE "=" ";" LEFT_RIGHT_EQ ${DEF_ROW})
        list(GET LEFT_RIGHT_EQ 1 ELEM2) # default value
        list(GET LEFT_RIGHT_EQ 0 TYPE_AND_NAME) # type and name

        string(REPLACE "?" ";" TYPE_AND_NAME ${TYPE_AND_NAME})
        list(GET TYPE_AND_NAME 0 ELEM1) # type
        list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        #remove leading and trailing spaces
        string(STRIP ${ELEM0} ELEM0)
        string(STRIP ${ELEM1} ELEM1)
        string(STRIP ${ELEM2} ELEM2)

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
        #string(APPEND COMPONENT_FIELD_OFFSETS "\n        offsetof(${COMPONENT_NAME}, ${ELEM1}),")
        #string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n            case ${COMPONENT_FIELD_COUNT}: return (uint8_t*)(this) + offsetof(${COMPONENT_NAME}, ${ELEM1});")
        string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n            case ${COMPONENT_FIELD_COUNT}: return &${ELEM1};")
        string(APPEND COMPONENT_FIELD_NAMES "\n        \"${ELEM1}\",")

        string(APPEND COMPONENT_VARS "\n    // Row ${COMPONENT_FIELD_COUNT}, Size ${COMPONENT_FIELD_ELEMENT_SIZE}
    ${ELEM0} ${ELEM1} = ${ELEM2};")

        # Count up the COMPONENT_FIELD_COUNT, there is probably far better way for adding...
        math(EXPR COMPONENT_FIELD_COUNT "${COMPONENT_FIELD_COUNT} + 1" OUTPUT_FORMAT DECIMAL)

    endif()
endforeach()
endfunction(PARSE_DEF_FILE)
#message(STATUS "STR: ${COMPONENT_DEF_STR}")
