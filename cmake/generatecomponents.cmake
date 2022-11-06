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
set(READ_STATE_COMPONENT_INCLUDES 30)
set(READ_STATE_ENTITY_INCLUDES 31)
set(READ_STATE_ENUM_BEGIN 40)
set(READ_STATE_ENUM_VALUES 41)

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
#include <components/components.h>
#include <core/json.h>
#include <core/writejson.h>
")

set(HEADER_FILE_WRITE "${MY_HEADER_FIRST_LINE}
#pragma once
${MY_FILE_HEADER}")

file(WRITE "${FILENAME_TO_MODIFY}_systems.h" "${HEADER_FILE_WRITE}
#include <atomic>
#include <mutex>
#include <vector>\n")
file(WRITE "${FILENAME_TO_MODIFY}_components.h" "${HEADER_FILE_WRITE}")

set(CPP_FILE_WRITE "${MY_HEADER_FIRST_LINE}
#include \"generated_components.h\"
#include \"generated_systems.h\"
${MY_FILE_HEADER}
#include \"imgui.h\"\n")

file(WRITE "${FILENAME_TO_MODIFY}_systems.cpp" "${CPP_FILE_WRITE}")
file(WRITE "${FILENAME_TO_MODIFY}_components.cpp" "${CPP_FILE_WRITE}")


# Loop through each line in the DEF file.
foreach(DEF_ROW ${DEF_LIST})
    #remove trailing and leading spaces
    string(STRIP "${DEF_ROW}" DEF_ROW)

    ############################# INCLUDES ##################################################


    if(DEF_ROW STREQUAL "ComponentHeaderBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_COMPONENT_INCLUDES})
    elseif(DEF_ROW STREQUAL "ComponentHeaderEnd" AND READ_STATE EQUAL READ_STATE_COMPONENT_INCLUDES)
        set(READ_STATE ${READ_STATE_NONE})
    elseif(DEF_ROW STREQUAL "EntityHeaderBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_INCLUDES})
    elseif(DEF_ROW STREQUAL "EntityHeaderEnd" AND READ_STATE EQUAL READ_STATE_ENTITY_INCLUDES)
        set(READ_STATE ${READ_STATE_NONE})
    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_INCLUDES)
        file(APPEND "${FILENAME_TO_MODIFY}_components.h" "${DEF_ROW}\n")
    elseif(READ_STATE EQUAL READ_STATE_ENTITY_INCLUDES)
        file(APPEND "${FILENAME_TO_MODIFY}_systems.h" "${DEF_ROW}\n")

################################### PARSE ENTITY ############################################

    elseif(DEF_ROW STREQUAL "EntityEnd" AND READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

       ####################### ENTITY HEADER ############################

        file(APPEND "${FILENAME_TO_MODIFY}_systems.h" "
struct ${ENTITY_NAME}
{
    ~${ENTITY_NAME}();

    struct ${ENTITY_NAME}ComponentArrayHandleBuilder
    {
        ${ENTITY_NAME}ComponentArrayHandleBuilder& addComponent(ComponentType componentType);

        u64 componentIndexArray = 0;
    };

    struct ${ENTITY_NAME}EntityLockedMutexHandle
    {
        u64 lockIndex = 0;
    };

    static constexpr ComponentType componentTypes[] =
    {${ENTITY_COMPONENT_TYPES_ARRAY}
    };

    static constexpr const char* entitySystemName = \"${ENTITY_NAME}\";
    static constexpr EntitySystemType entitySystemID = ${ENTITY_ID};
    static constexpr u32 entityVersion = ${ENTITY_VERSION};
    static constexpr u32 componentTypeCount = sizeof(componentTypes) / sizeof(ComponentType);

    static u32 getComponentIndex(ComponentType componentType);

    bool hasComponent(EntitySystemHandle handle, ComponentType componentType) const;
    bool hasComponents(EntitySystemHandle handle,
        const ${ENTITY_NAME}ComponentArrayHandleBuilder& componentArrayHandle) const;

    bool hasComponent(u32 entityIndex, ComponentType componentType) const;
    bool hasComponents(u32 entityIndex, const ${ENTITY_NAME}ComponentArrayHandleBuilder& componentArrayHandle) const;
    bool hasComponents(u32 entityIndex, const EntityRWHandle &rwHandle) const;

    // Different handle types for getting array... These are needed to set atomic locks...
    static constexpr ${ENTITY_NAME}ComponentArrayHandleBuilder getComponentArrayHandleBuilder() { return ${ENTITY_NAME}ComponentArrayHandleBuilder(); }
    const EntityRWHandle getRWHandle(
        const ${ENTITY_NAME}ComponentArrayHandleBuilder& readBuilder,
        const ${ENTITY_NAME}ComponentArrayHandleBuilder& writeBuilder);

    ${ENTITY_NAME}EntityLockedMutexHandle getLockedMutexHandle();
    bool releaseLockedMutexHandle(const ${ENTITY_NAME}EntityLockedMutexHandle& handle);

    bool syncReadWrites();

    EntitySystemHandle getEntitySystemHandle(u32 index) const;
    EntitySystemHandle addEntity(const ${ENTITY_NAME}EntityLockedMutexHandle& handle);
    bool removeEntity(EntitySystemHandle handle, const ${ENTITY_NAME}EntityLockedMutexHandle &mutexHandle);

    ${ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS}
${ENTITY_ADD_COMPONENT_HEADER}

    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json, const ${ENTITY_NAME}EntityLockedMutexHandle &mutexHandle);
    u32 getEntityCount() const { return (u32)entityComponents.size(); }

    void imguiRenderEntity();

private:${ENTITY_ARRAYS_FIELD}
    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(componentTypeCount < 64, \"Only 64 components are allowed for entity!\");

    std::mutex entityAddRemoveMutex {};
    u64 mutexLockIndex = 0;

    std::atomic<u64> readArrays {0};
    std::atomic<u64> writeArrays {0};
    u32 currentSyncIndex = 0;

    bool entitiesAdded = false;
    bool entitiesRemoved = false;
};\n")


        ####################### ENTITY CPP ########################

        file(APPEND "${FILENAME_TO_MODIFY}_systems.cpp" "

${ENTITY_NAME}::${ENTITY_NAME}ComponentArrayHandleBuilder& ${ENTITY_NAME}::${ENTITY_NAME}ComponentArrayHandleBuilder::addComponent(ComponentType componentType)
{
    u32 componentIndex = ${ENTITY_NAME}::getComponentIndex(componentType);
    if(componentIndex >= ${ENTITY_NAME}::componentTypeCount)
    {
        ASSERT(componentIndex < ${ENTITY_NAME}::componentTypeCount);
        return *this;
    }
    componentIndexArray |= u64(1) << u64(componentIndex);
    return *this;
}

${ENTITY_NAME}::~${ENTITY_NAME}()
{
    syncReadWrites();
}

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

bool ${ENTITY_NAME}::hasComponents(EntitySystemHandle handle,
    const ${ENTITY_NAME}ComponentArrayHandleBuilder& componentArrayHandle) const
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

bool ${ENTITY_NAME}::hasComponent(u32 entityIndex, ComponentType componentType) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    u64 componentIndex = getComponentIndex(componentType);

    if(componentIndex >= componentTypeCount)
        return false;

    return ((entityComponents[entityIndex] >> componentIndex) & 1) == 1;
}

bool ${ENTITY_NAME}::hasComponents(u32 entityIndex,
    const ${ENTITY_NAME}ComponentArrayHandleBuilder& componentArrayHandle) const
{
    if(entityIndex >= entityComponents.size())
        return false;

    auto compIndArr = componentArrayHandle.componentIndexArray;
    return (entityComponents[entityIndex] & compIndArr) == compIndArr;
}

bool ${ENTITY_NAME}::hasComponents(u32 entityIndex, const EntityRWHandle &rwHandle) const
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

const EntityRWHandle ${ENTITY_NAME}::getRWHandle(
    const ${ENTITY_NAME}ComponentArrayHandleBuilder& readBuilder,
    const ${ENTITY_NAME}ComponentArrayHandleBuilder& writeBuilder)
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

EntitySystemHandle ${ENTITY_NAME}::getEntitySystemHandle(u32 index) const
{
    if(index >= entityComponents.size())
        return EntitySystemHandle();

    return EntitySystemHandle {
        .entitySystemType = entitySystemID,
        .entityIndexVersion = entityVersions[index],
        .entityIndex = index };
}

${ENTITY_NAME}::${ENTITY_NAME}EntityLockedMutexHandle ${ENTITY_NAME}::getLockedMutexHandle()
{
    entityAddRemoveMutex.lock();
    ++mutexLockIndex;
    return ${ENTITY_NAME}::${ENTITY_NAME}EntityLockedMutexHandle { .lockIndex = mutexLockIndex };
}

bool ${ENTITY_NAME}::releaseLockedMutexHandle(const ${ENTITY_NAME}::${ENTITY_NAME}EntityLockedMutexHandle& handle)
{
    if(mutexLockIndex != handle.lockIndex)
        return false;

    ++mutexLockIndex;
    entityAddRemoveMutex.unlock();
    return true;
}

bool ${ENTITY_NAME}::syncReadWrites()
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

EntitySystemHandle ${ENTITY_NAME}::addEntity(const ${ENTITY_NAME}EntityLockedMutexHandle& handle)
{
    u32 addIndex = ~0u;

    if(freeEntityIndices.size() == 0)
    {${ENTITY_ARRAY_PUSHBACKS}
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

bool ${ENTITY_NAME}::removeEntity(EntitySystemHandle handle, const ${ENTITY_NAME}EntityLockedMutexHandle &mutexHandle)
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
${ENTITY_COMPONENT_ARRAY_GETTERS}
${ENTITY_ADD_COMPONENT}
bool ${ENTITY_NAME}::serialize(WriteJson &json) const
{
    u32 entityAmount = entityComponents.size();
    if(entityAmount == 0)
        return false;

    json.addObject(entitySystemName);
    json.addString(\"EntitySystemName\", entitySystemName);
    json.addInteger(\"EntitySystemTypeId\", u32(entitySystemID));
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

bool ${ENTITY_NAME}::deserialize(const JsonBlock &json, const ${ENTITY_NAME}EntityLockedMutexHandle &mutexHandle)
{
    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    const JsonBlock& child = json.getChild(entitySystemName);
    if(!child.isValid())
        return false;

    if(!child.getChild(\"EntitySystemTypeId\").equals(u32(entitySystemID)) || !child.getChild(\"EntitySystemName\").equals(entitySystemName))
        return false;

    u32 addedCount = 0u;
    for(const auto &entityJson : child.getChild(\"Entities\"))
    {
        addEntity(mutexHandle);
        for(const auto &obj : entityJson.getChild(\"Components\"))
        {${ENTITY_LOAD_CONTENTS}
        }
        addedCount++;
    }
    return true;
}

void ${ENTITY_NAME}::imguiRenderEntity()
{
    auto builder = getComponentArrayHandleBuilder();
    builder.componentIndexArray = (u64(1) << u64(componentTypeCount)) - 1;

    auto rwhandle = getRWHandle({}, builder);

${ENTITY_COMPONENT_ARRAY_GETTING_IMGUI}
    //ImGui::Begin(\"Entity\");
    ImGui::Begin(\"${ENTITY_NAME}\");
    ImGui::Text(\"${ENTITY_NAME}\");
    for(u32 i = 0; i < entityComponents.size(); ++i)
    {
        ImGui::Text(\"    [%i]\", i);
        if(entityComponents[i] == 0)
            continue;

        //ImGui::Begin(\"Entity2\");
        auto handle = getEntitySystemHandle(i);
        ${ENTITY_WRITE_IMGUI_CONTENTS}
        //ImGui::End();
    }
    ImGui::End();

}


")



    elseif(DEF_ROW STREQUAL "EntityBegin"  AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_BEGIN})
        set(ENTITY_LOAD_CONTENTS "")
        set(ENTITY_WRITE_CONTENTS "")
        set(ENTITY_WRITE_IMGUI_CONTENTS "")
        set(ENTITY_ARRAYS_FIELD "")
        set(ENTITY_ARRAY_PUSHBACKS "")
        set(ENTITY_ADD_COMPONENT "")
        set(ENTITY_ADD_COMPONENT_HEADER "")
        set(ENTITY_COMPONENT_TYPES_ARRAY "")
        set(ENTITY_COMPONENT_ARRAY_GETTING_IMGUI "")
        set(ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS "const u64* getComponentsReadArray() const;")
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

        set(ENTITY_COMPONENT_ARRAY_GETTERS "
const u64* ${ENTITY_NAME}::getComponentsReadArray() const
{
    return entityComponents.data();
}\n")



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
        string(APPEND ENTITY_WRITE_IMGUI_CONTENTS "
        if(hasComponent(handle, ${ELEM0}::componentID))
        {
            ImGui::Text(\"        ${ELEM0}\");
            array${ELEM0}[i].imguiRenderComponent();
        }")
    string(APPEND ENTITY_COMPONENT_ARRAY_GETTING_IMGUI
"    ${ELEM0}* array${ELEM0} = get${ELEM1}WriteArray(rwhandle);
    if(array${ELEM0} == nullptr)
        return;\n\n")
    string(APPEND ENTITY_COMPONENT_ARRAY_GETTERS_HEADERS "
    const ${ELEM0}* get${ELEM1}ReadArray(const EntityRWHandle& handle) const;
    ${ELEM0}* get${ELEM1}WriteArray(const EntityRWHandle& handle);")
        string(APPEND ENTITY_COMPONENT_ARRAY_GETTERS "
const ${ELEM0}* ${ENTITY_NAME}::get${ELEM1}ReadArray(const EntityRWHandle& handle) const
{
    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
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
    return ${ELEM1}Array.data();
}
${ELEM0}* ${ENTITY_NAME}::get${ELEM1}WriteArray(const EntityRWHandle& handle)
{
    u64 componentIndex = getComponentIndex(${ELEM0}::componentID);
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
    return ${ELEM1}Array.data();
}")

        string(APPEND ENTITY_COMPONENT_TYPES_ARRAY "\n        ${ELEM0}::componentID,")
        string(APPEND ENTITY_ARRAY_PUSHBACKS "\n        ${ELEM1}Array.emplace_back();")
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
        string(APPEND ENTITY_ADD_COMPONENT_HEADER "    bool add${ELEM0}(EntitySystemHandle handle, const ${ELEM0}& component);\n")
        string(APPEND ENTITY_ADD_COMPONENT "
bool ${ENTITY_NAME}::add${ELEM0}(EntitySystemHandle handle, const ${ELEM0}& component)
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


        #################### COMPONENT HEADER #####################

        file(APPEND "${FILENAME_TO_MODIFY}_components.h" "
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
    void imguiRenderComponent();

public:
    void* getElementIndexRef(u32 index);
    const void* getElementIndex(u32 index) const;
};\n")


        ################### COMPONENT CPP ######################

        file(APPEND "${FILENAME_TO_MODIFY}_components.cpp" "
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

void ${COMPONENT_NAME}::imguiRenderComponent()
{${COMPONENT_PRINT_IMGUI_PRINT_FIELDS}
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

    elseif(DEF_ROW STREQUAL "ComponentBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_COMPONENT_BEGIN})
        set(COMPONENT_FIELD_COUNT 0)
        set(COMPONENT_VARS "")
        set(COMPONENT_FIELD_TYPES "")
        set(COMPONENT_FIELD_NAMES "")
        set(COMPONENT_ELEMENT_GET_FUNC "")
        set(COMPONENT_PRINT_IMGUI_PRINT_FIELDS "")

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
        list(GET TYPE_AND_NAME 0 ELEM1) # fieldname
        list(GET TYPE_AND_NAME 1 ELEM0) # type

        #remove leading and trailing spaces
        string(STRIP "${ELEM0}" ELEM0)
        string(STRIP "${ELEM1}" ELEM1)
        string(STRIP "${ELEM2}" ELEM2)

        set(ELEM0_LEN 0)
        set(COMPONENT_FIELD_ELEMENT_SIZE "sizeof(${ELEM0})")
        if(ELEM0 STREQUAL "bool")
            set(COMPONENT_FIELD_ELEMENT_SIZE 1)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::BoolType,")
        elseif(ELEM0 STREQUAL "i8")
            set(COMPONENT_FIELD_ELEMENT_SIZE 1)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::I8Type,")
        elseif(ELEM0 STREQUAL "u8")
            set(COMPONENT_FIELD_ELEMENT_SIZE 1)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::U8Type,")
        elseif(ELEM0 STREQUAL "i16")
            set(COMPONENT_FIELD_ELEMENT_SIZE 2)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::I16Type,")
        elseif(ELEM0 STREQUAL "u16")
            set(COMPONENT_FIELD_ELEMENT_SIZE 2)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::U16Type,")
        elseif(ELEM0 STREQUAL "int" OR ELEM0 STREQUAL "i32")
            set(COMPONENT_FIELD_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::I32Type,")
        elseif(ELEM0 STREQUAL "u32")
            set(COMPONENT_FIELD_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::U32Type,")
        elseif(ELEM0 STREQUAL "i64")
            set(COMPONENT_FIELD_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::I64Type,")
        elseif(ELEM0 STREQUAL "u64")
            set(COMPONENT_FIELD_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::U64Type,")
        elseif(ELEM0 STREQUAL "float" OR ELEM0 STREQUAL "f32")
            set(COMPONENT_FIELD_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::F32Type,")
        elseif(ELEM0 STREQUAL "double" OR ELEM0 STREQUAL "f64")
            set(COMPONENT_FIELD_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::F64Type,")
        elseif(ELEM0 STREQUAL "Vector2")
            set(COMPONENT_FIELD_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec2Type,")
        elseif(ELEM0 STREQUAL "Vector3")
            set(COMPONENT_FIELD_ELEMENT_SIZE 12)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec3Type,")
        elseif(ELEM0 STREQUAL "Vector4")
            set(COMPONENT_FIELD_ELEMENT_SIZE 16)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec4Type,")
        elseif(ELEM0 STREQUAL "Quat")
            set(COMPONENT_FIELD_ELEMENT_SIZE 16)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::QuatType,")
        elseif(ELEM0 STREQUAL "Mat3x4")
            set(COMPONENT_FIELD_ELEMENT_SIZE 48)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Mat3x4Type,")
        elseif(ELEM0 STREQUAL "Matrix")
            set(COMPONENT_FIELD_ELEMENT_SIZE 64)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Mat4Type,")
        else()
            #string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::U32Type,")
            string(REPLACE "-" ";" ELEM0 ${ELEM0})
            set(ENUM_TYPE false)
            list(LENGTH ELEM0 ELEM0_LEN)
            if(ELEM0_LEN EQUAL 2)
                list(GET ELEM0 0 ELEM_TYPE) # type type
                list(GET ELEM0 1 ELEM0) # fieldname

                string(STRIP "${ELEM0}" ELEM0)
                string(STRIP "${ELEM_TYPE}" ELEM_TYPE)
                if(ELEM_TYPE STREQUAL "enum")
                    string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::EnumType,")
                endif()
            endif()
        endif()
        if(ELEM0_LEN EQUAL 2)
            string(APPEND COMPONENT_PRINT_IMGUI_PRINT_FIELDS "
    imguiPrintField(fieldNames[${COMPONENT_FIELD_COUNT}], getElementIndexRef(${COMPONENT_FIELD_COUNT}), fieldTypes[${COMPONENT_FIELD_COUNT}],
        ${ELEM0}::enumNames, ${ELEM0}::enumFieldCount);")
        else()
            string(APPEND COMPONENT_PRINT_IMGUI_PRINT_FIELDS "
    imguiPrintField(fieldNames[${COMPONENT_FIELD_COUNT}], getElementIndexRef(${COMPONENT_FIELD_COUNT}), fieldTypes[${COMPONENT_FIELD_COUNT}]);")
        endif()
        string(APPEND COMPONENT_ELEMENT_GET_FUNC "
            case ${COMPONENT_FIELD_COUNT}: return &${ELEM1};")
        string(APPEND COMPONENT_FIELD_NAMES "\n        \"${ELEM1}\",")

        string(APPEND COMPONENT_VARS "
    // Row ${COMPONENT_FIELD_COUNT}, Size ${COMPONENT_FIELD_ELEMENT_SIZE}
    ${ELEM0} ${ELEM1} = ${ELEM2};")

        # Count up the COMPONENT_FIELD_COUNT, there is probably far better way for adding...
        math(EXPR COMPONENT_FIELD_COUNT "${COMPONENT_FIELD_COUNT} + 1" OUTPUT_FORMAT DECIMAL)







    elseif(DEF_ROW STREQUAL "EnumBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENUM_BEGIN})

    elseif(READ_STATE EQUAL READ_STATE_ENUM_BEGIN)
        set(READ_STATE ${READ_STATE_ENUM_VALUES})
        set(ENUM_NAME ${DEF_ROW})
        set(ENUM_VALUES "")
        set(ENUM_VALUES_STRING "")
        set(ENUM_VALUE_COUNT 0)
        set(ENUM_VALUES_STRING "")
        set(ENUM_VALUES "")

    elseif(DEF_ROW STREQUAL "EnumEnd" AND READ_STATE EQUAL READ_STATE_ENUM_VALUES)
        set(READ_STATE ${READ_STATE_NONE})

        file(APPEND "${FILENAME_TO_MODIFY}_components.h" "
struct ${ENUM_NAME}
{
    enum : u32
    {
${ENUM_VALUES}
        EnumValueCount
    } enumValue;

    static constexpr const char *const enumNames[] =
    {
${ENUM_VALUES_STRING}
        \"EnumValueCount\"
    };

    static constexpr u32 enumFieldCount = ${ENUM_VALUE_COUNT};
};\n")

    elseif(READ_STATE EQUAL READ_STATE_ENUM_VALUES)

        math(EXPR ENUM_VALUE_COUNT "${ENUM_VALUE_COUNT} + 1" OUTPUT_FORMAT DECIMAL)


        string(APPEND ENUM_VALUES
"        ${DEF_ROW},\n")
        string(APPEND ENUM_VALUES_STRING
"        \"${DEF_ROW}\",\n")
    endif()
endforeach()
#endfunction(PARSE_DEF_FILE)
