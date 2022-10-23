set(READ_STATE_NONE 0)
set(READ_STATE_COMPONENT_BEGIN 1)
set(READ_STATE_COMPONENT_FIELDS 2)
#set(READ_STATE_COMPONENT_END 3)
set(READ_STATE_ENTITY_BEGIN 10)
set(READ_STATE_ENTITY_FIELDS 11)
#set(READ_STATE_ENTITY_END 12)

# Set our reading state to 0, there is probably a lot better way
set(READ_STATE ${READ_STATE_NONE})
set(COMPONENT_ROW_COUNT 0)
set(FILENAME_TO_MODIFY ${CMAKE_CURRENT_LIST_DIR}/generated_header.h)
# Read the entire DEF file.
file(READ ${CMAKE_CURRENT_LIST_DIR}/test.def DEF_CONTENTS)

# Split the DEF-file by new-lines.
string(REPLACE "\n" ";" DEF_LIST ${DEF_CONTENTS})

set(COMPONENT_DEF_STR "")

#not sure if this write + append is good....
file(WRITE ${FILENAME_TO_MODIFY}
"#pragma once

// This is generated file, do not modify.

#include \"src/components_macro.h\"
#include <math/vector3.h>\n")

# Loop through each line in the DEF file.
foreach(DEF_ROW ${DEF_LIST})
    #remove trailing and leading spaces
    string(STRIP ${DEF_ROW} DEF_ROW)


    if(DEF_ROW STREQUAL "EntityTypeEnd" AND READ_STATE_NONE EQUAL READ_STATE_ENTITY_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})

    elseif(DEF_ROW STREQUAL "EntityTypeBegin"  AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_ENTITY_BEGIN})


    elseif(READ_STATE EQUAL READ_STATE_ENTITY_BEGIN)
        set(READ_STATE ${READ_STATE_ENTITY_FIELDS})
        


    elseif(READ_STATE EQUAL READ_STATE_ENTITY_FIELDS)









    elseif(DEF_ROW STREQUAL "ComponentEnd" AND READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)
        set(READ_STATE ${READ_STATE_NONE})
        string(APPEND COMPONENT_FIELD_TYPES "\n    }\;")
        string(APPEND COMPONENT_FIELD_NAMES "\n    }\;")
        #string(APPEND COMPONENT_FIELD_OFFSETS "\n    }\;")
        string(APPEND COMPONENT_STATIC_VARS "    static constexpr unsigned int componentFieldAmount = ${COMPONENT_ROW_COUNT}\;")
        string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n
            default: ASSERT_STRING(false, \"Unknown index\")\;
        }
        return nullptr\;\n    }")

        string(APPEND COMPONENT_DEF_STR "\n${COMPONENT_STATIC_VARS}
${COMPONENT_VARS}\n
${COMPONENT_FIELD_TYPES}\n
${COMPONENT_FIELD_NAMES}\n
${COMPONENT_ELEMENT_GET_FUNC}\n}\;\n")

        file(APPEND ${FILENAME_TO_MODIFY} ${COMPONENT_DEF_STR})

    elseif(DEF_ROW STREQUAL "ComponentBegin" AND READ_STATE EQUAL READ_STATE_NONE)
        set(READ_STATE ${READ_STATE_COMPONENT_BEGIN})
        set(COMPONENT_ROW_COUNT 0)
        set(COMPONENT_DEF_STR "\nstruct ")
        set(COMPONENT_VARS "")
        #set(COMPONENT_FIELD_OFFSETS "    constexpr size_t fieldOffsets[] =\n    {")
        set(COMPONENT_FIELD_TYPES "    static constexpr FieldType fieldTypes[] =\n    {")
        set(COMPONENT_FIELD_NAMES "    static constexpr const char* fieldNames[] =\n    {")
        set(COMPONENT_ELEMENT_GET_FUNC "    void* getElementIndex(unsigned int index)
    {
        switch(index)
        {")
#        return (uint8_t*)(this) + fieldOffsets[index]\;
#    }")


    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_BEGIN)
        set(READ_STATE ${READ_STATE_COMPONENT_FIELDS})

        # Get a list of the elements in this CSV row.
        string(REPLACE " " ";" DEF_ROW_CONTENTS ${DEF_ROW})
        #message(STATUS "contents = ${DEF_ROW_CONTENTS}")


        #list(LENGTH DEF_ROW_CONTENTS len)
        #message(STATUS "len = ${len}")
        # len = 3

        # Get variables to each element.

        list(GET DEF_ROW_CONTENTS 0 ELEM0) # component name
        list(GET DEF_ROW_CONTENTS 1 ELEM1) # component id
        list(GET DEF_ROW_CONTENTS 2 ELEM2) # component version

        string(APPEND COMPONENT_DEF_STR "${ELEM0}\n{")
        set(COMPONENT_NAME ${ELEM0})
        set(COMPONENT_STATIC_VARS
"    static constexpr const char* componentName = \"${ELEM0}\"\;
    static constexpr unsigned int componentID = ${ELEM1}\;
    static constexpr unsigned int componentVersion = ${ELEM2}\;\n")

    elseif(READ_STATE EQUAL READ_STATE_COMPONENT_FIELDS)

        # Get a list of the elements in this CSV row.
        string(REPLACE "=" ";" LEFT_RIGHT_EQ ${DEF_ROW})
        list(GET LEFT_RIGHT_EQ 1 ELEM2) # default value
        list(GET LEFT_RIGHT_EQ 0 TYPE_AND_NAME) # type and name

        string(REPLACE ":" ";" TYPE_AND_NAME ${TYPE_AND_NAME})
        list(GET TYPE_AND_NAME 0 ELEM1) # type
        list(GET TYPE_AND_NAME 1 ELEM0) # fieldname

        #remove leading and trailing spaces
        string(STRIP ${ELEM0} ELEM0)
        string(STRIP ${ELEM1} ELEM1)
        string(STRIP ${ELEM2} ELEM2)

        set(COMPONENT_ROW_ELEMENT_SIZE 0)
        if(ELEM0 STREQUAL "int")
            set(COMPONENT_ROW_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::IntType,")
        elseif(ELEM0 STREQUAL "float")
            set(COMPONENT_ROW_ELEMENT_SIZE 4)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::FloatType,")
        elseif(ELEM0 STREQUAL "Vector2")
            set(COMPONENT_ROW_ELEMENT_SIZE 8)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec2Type,")
        elseif(ELEM0 STREQUAL "Vector3")
            set(COMPONENT_ROW_ELEMENT_SIZE 12)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec3Type,")
        elseif(ELEM0 STREQUAL "Vector4")
            set(COMPONENT_ROW_ELEMENT_SIZE 16)
            string(APPEND COMPONENT_FIELD_TYPES "\n        FieldType::Vec4Type,")
        endif()
        #string(APPEND COMPONENT_FIELD_OFFSETS "\n        offsetof(${COMPONENT_NAME}, ${ELEM1}),")
        #string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n            case ${COMPONENT_ROW_COUNT}: return (uint8_t*)(this) + offsetof(${COMPONENT_NAME}, ${ELEM1})\;")
        string(APPEND COMPONENT_ELEMENT_GET_FUNC "\n            case ${COMPONENT_ROW_COUNT}: return &${ELEM1}\;")
        string(APPEND COMPONENT_FIELD_NAMES "\n        \"${ELEM1}\",")

        string(APPEND COMPONENT_VARS "\n    // Row ${COMPONENT_ROW_COUNT}, Size ${COMPONENT_ROW_ELEMENT_SIZE}
    ${ELEM0} ${ELEM1} = ${ELEM2}\;")

            # Count up the VALUE_OUT, there is probably far better way for adding...
        math(EXPR COMPONENT_ROW_COUNT "${COMPONENT_ROW_COUNT} + 1" OUTPUT_FORMAT DECIMAL)

    endif()
endforeach()

#message(STATUS "STR: ${COMPONENT_DEF_STR}")
