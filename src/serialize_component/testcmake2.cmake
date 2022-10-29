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
set(MY_HEADER_FIRST_LINE "// This is generated file, do not modify.")
set(MY_FILE_HEADER "
#include \"src/components.h\"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <atomic>
#include <vector>\n")

file(WRITE "${FILENAME_TO_MODIFY}.h" "${MY_HEADER_FIRST_LINE}
#pragma once
${MY_FILE_HEADER}")
file(WRITE "${FILENAME_TO_MODIFY}.cpp" "${MY_HEADER_FIRST_LINE}
#include \"generated_header.h\"
${MY_FILE_HEADER}")


# Loop through each line in the DEF file.
foreach(DEF_ROW ${DEF_LIST})
    #remove trailing and leading spaces
    string(STRIP "${DEF_ROW}" DEF_ROW)


################################### PARSE ENTITY ############################################

    if(DEF_ROW STREQUAL "EntityTypeEnd" AND READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

####################### ENTITY CPP ########################

        file(APPEND "${FILENAME_TO_MODIFY}.cpp" "

bool ${ENTITY_NAME}::hasComponent(EntitySystemHandle handle, ComponentType componentType) const
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

u32 ${ENTITY_NAME}::getComponentIndex(ComponentType componentType)
{
    // Could be written with switch-cases if it comes to that. Probably no need to though
    for(u32 i = 0; i < componentTypeCount; ++i)
    {
        if(componentType == componentTypes[i])
            return i;
    }
    return ~0u;
}

const EntityReadWriteHandle ${ENTITY_NAME}::getReadWriteHandle(const ${ENTITY_NAME}ReadWriteHandleBuilder& builder)
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

EntitySystemHandle ${ENTITY_NAME}::getEntitySystemHandle(u32 index) const
{
    if(index >= entityComponents.size())
        return EntitySystemHandle();

    return EntitySystemHandle {
        .entitySystemType = entitySystemID,
        .entityIndexVersion = entityVersions[index],
        .entityIndex = index };
}

EntitySystemHandle ${ENTITY_NAME}::addEntity()
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

bool ${ENTITY_NAME}::removeEntity(EntitySystemHandle handle)
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
${ENTITY_COMPONENT_ARRAY_GETTERS}
${ENTITY_ADD_COMPONENT}
bool ${ENTITY_NAME}::serialize(WriteJson &json) const
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

bool ${ENTITY_NAME}::deserialize(const JsonBlock &json)
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

")




####################### ENTITY HEADER ############################

        file(APPEND "${FILENAME_TO_MODIFY}.h" "
struct ${ENTITY_NAME}
{
    struct ${ENTITY_NAME}ReadWriteHandleBuilder
    {
        ${ENTITY_NAME}ReadWriteHandleBuilder& addArrayRead(ComponentType componentType)
        {
            u32 componentIndex = ${ENTITY_NAME}::getComponentIndex(componentType);
            if(componentIndex < 0 || componentIndex >= ${ENTITY_NAME}::componentTypeCount)
            {
                ASSERT(componentIndex > 0 && componentIndex < ${ENTITY_NAME}::componentTypeCount);
                return *this;
            }
            readArrays |= u64(1) << u64(componentIndex); 
            return *this;
        }
        
        ${ENTITY_NAME}ReadWriteHandleBuilder& addArrayWrite(ComponentType componentType)
        {
            u32 componentIndex = ${ENTITY_NAME}::getComponentIndex(componentType);
            if(componentIndex < 0 || componentIndex >= ${ENTITY_NAME}::componentTypeCount)
            {
                ASSERT(componentIndex > 0 && componentIndex < ${ENTITY_NAME}::componentTypeCount);
                return *this;
            }
            writeArrays |= u64(1) << u64(componentIndex); 
            return *this;
        }
        u64 readArrays = 0;
        u64 writeArrays = 0;
    };
    
    static constexpr ComponentType componentTypes[] =
    {${ENTITY_COMPONENT_TYPES_ARRAY}
    };
    
    static constexpr const char* entitySystemName = \"${ENTITY_NAME}\";
    static constexpr EntityType entitySystemID = ${ENTITY_ID};
    static constexpr u32 entityVersion = ${ENTITY_VERSION};
    static constexpr u32 componentTypeCount = sizeof(componentTypes) / sizeof(ComponentType);

    static u32 getComponentIndex(ComponentType componentType);
    bool hasComponent(EntitySystemHandle handle, ComponentType componentType) const;

    // Different handle types for getting array... These are needed to set atomic locks...
    static constexpr ${ENTITY_NAME}ReadWriteHandleBuilder getReadWriteHandleBuilder() { return ${ENTITY_NAME}ReadWriteHandleBuilder(); }
    const EntityReadWriteHandle getReadWriteHandle(const ${ENTITY_NAME}ReadWriteHandleBuilder& builder);

    EntitySystemHandle getEntitySystemHandle(u32 index) const;
    EntitySystemHandle addEntity();
    bool removeEntity(EntitySystemHandle handle);
${ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS}
${ENTITY_ADD_COMPONENT_HEADER}
    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json);
    u32 getEntityCount() const { return (u32)entityComponents.size(); }

private:${ENTITY_ARRAYS_FIELD}

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(componentTypeCount < 64, \"Only 64 components are allowed for entity!\");
    
    std::atomic<u64> readArrays {0};
    std::atomic<u64> writeArrays {0};
    u32 currentSyncIndex = 0;
};\n")

    elseif(DEF_ROW STREQUAL "EntityTypeBegin"  AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_BEGIN})
        set(ENTITY_LOAD_CONTENTS "")
        set(ENTITY_WRITE_CONTENTS "")
        set(ENTITY_ARRAYS_FIELD "")
        set(ENTITY_ARRAY_PUSHBACKS "")
        set(ENTITY_ADD_COMPONENT "")
        set(ENTITY_ADD_COMPONENT_HEADER "")
        set(ENTITY_COMPONENT_TYPES_ARRAY "")
        set(ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS "")
        set(ENTITY_COMPONENT_ARRAY_GETTERS "")

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

        #list(GET TYPE_AND_NAME 0 ELEM1) # type
        #list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        set(ELEM0 "${TYPE_AND_NAME}")
        set(ELEM1 "${TYPE_AND_NAME}")

        #remove leading and trailing spaces
        string(STRIP "${ELEM0}" ELEM0)
        string(STRIP "${ELEM1}" ELEM1)

        string(APPEND ENTITY_WRITE_CONTENTS "
            if(hasComponent(getEntitySystemHandle(i), ${ELEM0}::componentID))
            {
                ${ELEM1}Array[i].serialize(json);
            }")
        string(APPEND ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS "
    const ${ELEM0}* get${ELEM1}ReadArray(const EntityReadWriteHandle& handle) const;
    ${ELEM0}* get${ELEM1}WriteArray(const EntityReadWriteHandle& handle);")
        string(APPEND ENTITY_COMPONENT_ARRAY_GETTERS "
const ${ELEM0}* ${ENTITY_NAME}::get${ELEM1}ReadArray(const EntityReadWriteHandle& handle) const
{
    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.readArrays >> componentIndex) & 1) == 0)
        )
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.readArrays >> u64(${ELEM0}::componentID)) & 1) == 1);
        return nullptr;
    }
    return ${ELEM1}Array.data();
}
${ELEM0}* ${ENTITY_NAME}::get${ELEM1}WriteArray(const EntityReadWriteHandle& handle)
{
    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
    ASSERT(componentIndex < componentTypeCount);

    if(componentIndex >= componentTypeCount ||
        handle.readWriteHandleTypeId != entitySystemID ||
        handle.syncIndexPoint != currentSyncIndex ||
        (((handle.writeArrays >> componentIndex) & 1) == 0))
    {
        ASSERT(handle.readWriteHandleTypeId == entitySystemID);
        ASSERT(handle.syncIndexPoint == currentSyncIndex);
        ASSERT(((handle.writeArrays >> u64(${ELEM0}::componentID)) & 1) == 1);
        return nullptr;
    }
    return ${ELEM1}Array.data();
}")

        string(APPEND ENTITY_COMPONENT_TYPES_ARRAY "\n        ${ELEM0}::componentID,")
        string(APPEND ENTITY_ARRAY_PUSHBACKS "\n            ${ELEM1}Array.emplace_back();")
        string(APPEND ENTITY_ARRAYS_FIELD "\n    std::vector<${ELEM0}> ${ELEM1}Array;")
        string(APPEND ENTITY_LOAD_CONTENTS "
                if(${ELEM1}Array[addedCount].deserialize(obj))
                {
                    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
                    if(componentIndex >= componentTypeCount)
                        return false;

                    entityComponents[addedCount] |= u64(1) << componentIndex;
                    continue;
                }")
        string(APPEND ENTITY_ADD_COMPONENT_HEADER "\n    bool add${ELEM0}Component(EntitySystemHandle handle, const ${ELEM0}& component);")
        string(APPEND ENTITY_ADD_COMPONENT "
bool ${ENTITY_NAME}::add${ELEM0}Component(EntitySystemHandle handle, const ${ELEM0}& component)
{
    // Some error if no lock

    if(handle.entitySystemType != entitySystemID)
        return false;

    if(handle.entityIndex >= entityComponents.size())
        return false;

    if(handle.entityIndexVersion != entityVersions[handle.entityIndex])
        return false;

    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);

    if(componentIndex >= componentTypeCount)
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

################### COMPONENT HEADER ######################

        file(APPEND "${FILENAME_TO_MODIFY}.cpp" "
bool ${COMPONENT_NAME}::serialize(WriteJson &json) const
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

bool ${COMPONENT_NAME}::deserialize(const JsonBlock &json)
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

void* ${COMPONENT_NAME}::getElementIndexRef(u32 index)
{
    switch(index)
    {${COMPONENT_ELEMENT_GET_FUNC}
        default: ASSERT_STRING(false, \"Unknown index\");
    }
    return nullptr;
}

const void* ${COMPONENT_NAME}::getElementIndex(u32 index) const
{
    switch(index)
    {${COMPONENT_ELEMENT_GET_FUNC}
        default: ASSERT_STRING(false, \"Unknown index\");
    }
    return nullptr;
}

")

#################### COMPONENT HEADER #####################

        file(APPEND "${FILENAME_TO_MODIFY}.h" "
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

    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json);

private:
    void* getElementIndexRef(u32 index);
    const void* getElementIndex(u32 index) const;
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
