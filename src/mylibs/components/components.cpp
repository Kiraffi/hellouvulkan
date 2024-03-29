#include "components.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/json.h>
#include <core/log.h>
#include <core/writejson.h>

#include <imgui.h>


bool serializeField(WriteJson &writeJson,
    const char* const fieldName,
    const void* const fieldMemoryAddress,
    FieldType fieldType)
{
    if(!writeJson.isValid() || fieldMemoryAddress == nullptr || fieldName == nullptr)
        return false;

    switch(fieldType)
    {
        case FieldType::BoolType:
        {
            bool b = *((bool*)fieldMemoryAddress);
            writeJson.addBool(fieldName, b);
            break;
        }

        case FieldType::I8Type:
        {
            i8 i = *((i8*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::U8Type:
        {
            u8 i = *((u8*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::I16Type:
        {
            i16 i = *((i16*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::U16Type:
        {
            u16 i = *((u16*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::I32Type:
        {
            i32 i = *((int*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::U32Type:
        {
            u32 i = *((u32*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::U64Type:
        case FieldType::I64Type:
        {
            i64 i = *((i64*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
            break;
        }
        case FieldType::F32Type:
        {
            float f = *((float*)fieldMemoryAddress);
            writeJson.addNumber(fieldName, f);
            break;
        }
        case FieldType::F64Type:
        {
            double d = *((double*)fieldMemoryAddress);
            writeJson.addNumber(fieldName, d);
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
        case FieldType::EnumType:
        {
            u32 i = *((u32*)fieldMemoryAddress);
            writeJson.addInteger(fieldName, i);
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
        case FieldType::BoolType:
        {
            bool &b = *((bool*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseBool(b))
                return false;
            break;
        }

        case FieldType::I8Type:
        {
            i8 &i2 = *((i8*)fieldMemoryAddress);
            i64 i = 0;
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            i2 = i8(i);
            break;
        }
        case FieldType::U8Type:
        {
            u8 &i2 = *((u8*)fieldMemoryAddress);
            i64 i = 0;
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            i2 = u8(i);
            break;
        }
        case FieldType::I16Type:
        {
            i16 &i2 = *((i16*)fieldMemoryAddress);
            i64 i = 0;
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            i2 = i16(i);
            break;
        }
        case FieldType::U16Type:
        {
            u16 &i2 = *((u16*)fieldMemoryAddress);
            i64 i = 0;
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            i2 = u16(i);
            break;
        }
        case FieldType::I32Type:
        {
            i32 &i = *((i32*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            break;
        }
        case FieldType::U32Type:
        {
            u32 &i = *((u32*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseUInt(i))
                return false;
            break;
        }
        case FieldType::U64Type:
        case FieldType::I64Type:
        {
            i64 &i = *((i64*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseInt(i))
                return false;
            break;
        }
        case FieldType::F32Type:
        {
            float &f = *((float*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseFloat(f))
                return false;
            break;
        }
        case FieldType::F64Type:
        {
            double &d = *((double*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseDouble(d))
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
        case FieldType::EnumType:
        {
            u32 &i = *((u32*)fieldMemoryAddress);
            if(!json.getChild(fieldName).parseUInt(i))
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
        case FieldType::BoolType:
        {
            bool b = *((bool *)fieldMemoryAddress);

            LOG("%s: %s\n", fieldName, b ? "true" : "false");
            break;
        }
        case FieldType::I8Type:
        {
            i32 i = *((i8 *)fieldMemoryAddress);
            LOG("%s: %i\n", fieldName, i);
            break;
        }
        case FieldType::U8Type:
        {
            i32 i = *((u8 *)fieldMemoryAddress);
            LOG("%s: %i\n", fieldName, i);
            break;
        }
        case FieldType::I16Type:
        {
            i32 i = *((i16 *)fieldMemoryAddress);
            LOG("%s: %i\n", fieldName, i);
            break;
        }
        case FieldType::U16Type:
        {
            i32 i = *((u16 *)fieldMemoryAddress);
            LOG("%s: %i\n", fieldName, i);
            break;
        }
        case FieldType::I32Type:
        {
            LOG("%s: %i\n", fieldName, *((i32 *)fieldMemoryAddress));
            break;
        }
        case FieldType::U32Type:
        {
            LOG("%s: %u\n", fieldName, *((u32 *)fieldMemoryAddress));
            break;
        }
        case FieldType::I64Type:
        {
            LOG("%s: %" PRIi64 "\n", fieldName, *((i64 *)fieldMemoryAddress));
            break;
        }
        case FieldType::U64Type:
        {
            LOG("%s: %" PRIu64 "\n", fieldName, *((u64 *)fieldMemoryAddress));
            break;
        }
        case FieldType::F32Type:
        {
            LOG("%s: %f\n", fieldName, *((f32 *)fieldMemoryAddress));
            break;
        }
        case FieldType::F64Type:
        {
            float f = f32(*((f64 *)fieldMemoryAddress));
            LOG("%s: %f\n", fieldName, f);
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
        case FieldType::EnumType:
        {

            break;
        }
        case FieldType::NumTypes:
        {
            break;
        }
    }
}

void imguiPrintField(const char* fieldName,
    void* fieldMemoryAddress,
    FieldType field)
{
    ASSERT(field != FieldType::EnumType);
    imguiPrintField(fieldName, fieldMemoryAddress, field, nullptr, 0);
}
void imguiPrintField(const char* fieldName,
    void* fieldMemoryAddress,
    FieldType field,
    const char* const* enumNames, u32 enumCount)
{
    ASSERT((field != FieldType::EnumType) ||
        (field == FieldType::EnumType && (enumCount == 0 || enumNames != nullptr)));
    if(fieldName == nullptr || fieldMemoryAddress == nullptr)
    {
        return;
    }
    if(field == FieldType::EnumType && enumNames == nullptr)
    {
        return;
    }
    static constexpr i64 steps = 1;
    static constexpr i64 fastSteps = 100;

    ImGui::PushID(fieldMemoryAddress);
    switch(field)
    {
        case FieldType::BoolType:
        {
            bool &b = *((bool *)fieldMemoryAddress);
            ImGui::Checkbox(fieldName, &b);
            break;
        }
        case FieldType::I8Type:
        {
            const char* format = "%d";
            ImGui::InputScalar(fieldName, ImGuiDataType_S8, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::U8Type:
        {
            const char* format = "%d";
            ImGui::InputScalar(fieldName, ImGuiDataType_U8, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::I16Type:
        {
            const char* format = "%d";
            ImGui::InputScalar(fieldName, ImGuiDataType_S16, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::U16Type:
        {
            const char* format = "%d";
            ImGui::InputScalar(fieldName, ImGuiDataType_U16, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::I32Type:
        {
            const char* format = "%d";
            ImGui::InputScalar(fieldName, ImGuiDataType_S32, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::U32Type:
        {
            const char* format = "%u";
            ImGui::InputScalar(fieldName, ImGuiDataType_U32, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::I64Type:
        {
            const char* format = "%" PRIi64;
            ImGui::InputScalar(fieldName, ImGuiDataType_S64, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::U64Type:
        {
            const char* format = "%" PRIu64;
            ImGui::InputScalar(fieldName, ImGuiDataType_U64, fieldMemoryAddress, &steps, &fastSteps, format, 0);
            break;
        }
        case FieldType::F32Type:
        {
            f32 *f = ((f32 *)fieldMemoryAddress);
            ImGui::InputFloat(fieldName, f);
            break;
        }
        case FieldType::F64Type:
        {
            f64 *d = ((f64 *)fieldMemoryAddress);
            ImGui::InputDouble(fieldName, d);
            break;
        }

        case FieldType::Vec2Type:
        {
            float *v = &(((Vector2 *)fieldMemoryAddress)->x);
            ImGui::InputFloat2(fieldName, v);
            break;
        }

        case FieldType::Vec3Type:
        {
            float *v = &(((Vector3 *)fieldMemoryAddress)->x);
            ImGui::InputFloat3(fieldName, v);
            break;
        }

        case FieldType::Vec4Type:
        {
            float *v = &(((Vector4 *)fieldMemoryAddress)->x);
            ImGui::InputFloat4(fieldName, v);
            break;
        }

        case FieldType::QuatType:
        {
            float *v = &(((Quat *)fieldMemoryAddress)->v.x);
            ImGui::InputFloat4(fieldName, v);
            break;
        }

        case FieldType::Mat3x4Type:
        {
            float *v = &((*((Mat3x4 *)fieldMemoryAddress))[0]);
            ImGui::InputFloat4(fieldName, v);
            break;
        }

        case FieldType::Mat4Type:
        {
            float *v = &((*((Matrix *)fieldMemoryAddress))[0]);
            ImGui::InputFloat4(fieldName, v);
            break;
        }
        case FieldType::EnumType:
        {
            u32 &v = *((u32 *)fieldMemoryAddress);
            const char* enumName = (v >= 0 && v < enumCount) ? enumNames[v] : "Unknown";
            //ImGui::SliderInt(fieldName, &i, 0, enumCount - 1, elem_name);
            if(ImGui::BeginCombo(fieldName, enumName, 0))
            {
                for(u32 i = 0; i < enumCount; ++i)
                {
                    bool isSelected = i == v;
                    if(ImGui::Selectable(enumNames[i], isSelected))
                    {
                        isSelected  = true;
                        v = i;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if(isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            break;
        }
        case FieldType::NumTypes:
        {
            break;
        }
    }
    ImGui::PopID();
}
