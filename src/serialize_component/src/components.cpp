#include "components.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/json.h>
#include <core/log.h>
#include <core/writejson.h>


bool serializeField(WriteJson &writeJson,
    const char* const fieldName,
    const void* const fieldMemoryAddress,
    FieldType fieldType)
{
    if(!writeJson.isValid() || fieldMemoryAddress == nullptr || fieldName == nullptr)
        return false;

    switch(fieldType)
    {
        case FieldType::IntType:
        {
            int i = *((int*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::FloatType:
        {
            float f = *((float*)fieldMemoryAddress);
            writeJson.addNumber(fieldName, f);
            break;
        }
        case FieldType::Vec2Type:
        {
            const Vector2 &v = *((Vector2*)fieldMemoryAddress);
            writeJson.writeVec2(fieldName, v);
            break;
        }
        case FieldType::Vec3Type:
        {
            const Vector3 &v = *((Vector3*)fieldMemoryAddress);
            writeJson.writeVec3(fieldName, v);
            break;
        }
        case FieldType::Vec4Type:
        {
            const Vector4 &v = *((Vector4*)fieldMemoryAddress);
            writeJson.writeVec4(fieldName, v);
            break;
        }

        case FieldType::QuatType:
        {
            const Quat &v = *((Quat*)fieldMemoryAddress);
            writeJson.writeQuat(fieldName, v);
            break;
        }

        case FieldType::Mat3x4Type:
        {
            //const Mat3x4 &m = *((Mat3x4*)fieldMemoryAddress);
            //writeJson.addNumberArray(fieldName, &m[0], 12);
            const float* const f = (const float* const)fieldMemoryAddress;
            writeJson.addNumberArray(fieldName, f, 12);
            break;
        }

        case FieldType::Mat4Type:
        {
            //const Matrix &m = *((Matrix*)fieldMemoryAddress);
            //writeJson.addNumberArray(fieldName, &m[0], 16);
            const float* const f = (const float* const)fieldMemoryAddress;
            writeJson.addNumberArray(fieldName, f, 16);
            break;
        }

        case FieldType::NumTypes:
        {
            ASSERT_STRING(false, "Unknown field type!");
        }
    }

    return writeJson.isValid();

}

bool deserializeField(const JsonBlock &json,
    const char* const fieldName,
    void* fieldMemoryAddress,
    FieldType fieldType)
{
    switch(fieldType)
    {
        case FieldType::IntType:
        {
            int &i = *((int*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            break;
        }
        case FieldType::FloatType:
        {
            float &f = *((float*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseFloat(f))
                return false;
            break;
        }
        case FieldType::Vec2Type:
        {
            Vector2 &v = *((Vector2*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseVec2(v))
                return false;
            break;
        }
        case FieldType::Vec3Type:
        {
            Vector3 &v = *((Vector3*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseVec3(v))
                return false;
            break;
        }
        case FieldType::Vec4Type:
        {
            Vector4 &v = *((Vector4*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseVec4(v))
                return false;
            break;
        }

        case FieldType::QuatType:
        {
            Quat &q = *((Quat*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseQuat(q))
                return false;
            break;
        }

        case FieldType::Mat3x4Type:
        {
            Mat3x4 &m = *((Mat3x4*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseNumberArray(&m[0], 12))
                return false;
            break;
        }
        case FieldType::Mat4Type:
        {
            Matrix &m = *((Matrix*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseNumberArray(&m[0], 16))
                return false;
            break;
        }

        case FieldType::NumTypes:
        {
            ASSERT_STRING(false, "Unknown field type!");
        }
    }
    return true;
}

void printFieldValue(const char* fieldName,
    const void* const fieldMemoryAddress,
    FieldType field)
{
    if(fieldName == nullptr || fieldMemoryAddress == nullptr)
    {
        return;
    }

    switch(field)
    {
        case FieldType::IntType:
        {
            LOG("%s: %i\n", fieldName, *((int *)fieldMemoryAddress));
            break;
        }
        case FieldType::FloatType:
        {
            LOG("%s: %f\n", fieldName, *((float *)fieldMemoryAddress));
            break;
        }

        case FieldType::Vec2Type:
        {
            const Vector2 &v = *((Vector2 *)fieldMemoryAddress);
            LOG("%s: [x: %f, y: %f]\n", fieldName, v.x, v.y);
            break;
        }

        case FieldType::Vec3Type:
        {
            const Vector3 &v = *((Vector3 *)fieldMemoryAddress);
            LOG("%s: [x: %f, y: %f, z: %f]\n", fieldName, v.x, v.y, v.z);
            break;
        }

        case FieldType::Vec4Type:
        {
            const Vector4 &v = *((Vector4 *)fieldMemoryAddress);
            LOG("%s: [x: %f, y: %f, z: %f, w: %f]\n", fieldName, v.x, v.y, v.z, v.w);
            break;
        }

        case FieldType::QuatType:
        {
            const Quat &q = *((Quat *)fieldMemoryAddress);
            LOG("%s: [x: %f, y: %f, z: %f, w: %f]\n", fieldName, q.v.x, q.v.y, q.v.z, q.w);
            break;
        }

        case FieldType::Mat3x4Type:
        {
            const Mat3x4 &m = *((Mat3x4 *)fieldMemoryAddress);
            LOG("%s\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n", fieldName,
                m[0], m[1], m[2], m[3],
                m[4], m[5], m[6], m[7],
                m[8], m[9], m[10], m[11]);
            break;
        }

        case FieldType::Mat4Type:
        {
            const Matrix &m = *((Matrix *)fieldMemoryAddress);
            LOG("%s\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n", fieldName,
                m[0], m[1], m[2], m[3],
                m[4], m[5], m[6], m[7],
                m[8], m[9], m[10], m[11],
                m[12], m[13], m[14], m[15]);
            break;
        }

        case FieldType::NumTypes:
        {
            break;
        }
    }
}
