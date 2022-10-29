// This is generated file, do not modify.
#pragma once

#include "src/components.h"

#include <core/json.h>
#include <core/writejson.h>
#include <math/vector3.h>

#include <atomic>
#include <mutex>
#include <vector>

struct TransformComponent
{
    static constexpr const char* componentName = "TransformComponent";
    static constexpr ComponentType componentID = ComponentType::TransformComponent;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 3;

    // Row 0, Size 16
    Vector4 position = {};
    // Row 1, Size 16
    Quat rotation = Quat{};
    // Row 2, Size 16
    Vector4 scale = {};

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::Vec4Type,
        FieldType::QuatType,
        FieldType::Vec4Type,
    };
    static constexpr const char* fieldNames[] =
    {
        "position",
        "rotation",
        "scale",
    };

    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json);

private:
    void* getElementIndexRef(u32 index);
    const void* getElementIndex(u32 index) const;
};

struct Heritaged1
{
    static constexpr const char* componentName = "Heritaged1";
    static constexpr ComponentType componentID = ComponentType::HeritagedType;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 6;

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
    // Row 5, Size 16
    Quat tempQuat = {};

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
        FieldType::Vec2Type,
        FieldType::Vec3Type,
        FieldType::Vec4Type,
        FieldType::QuatType,
    };
    static constexpr const char* fieldNames[] =
    {
        "tempInt",
        "tempFloat",
        "tempV2",
        "tempV3",
        "tempV4",
        "tempQuat",
    };

    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json);

private:
    void* getElementIndexRef(u32 index);
    const void* getElementIndex(u32 index) const;
};

struct Heritaged2
{
    static constexpr const char* componentName = "Heritaged2";
    static constexpr ComponentType componentID = ComponentType::HeritagedType2;
    static constexpr u32 componentVersion = 1;
    static constexpr u32 componentFieldAmount = 6;

    // Row 0, Size 4
    int tempInt = 98756;
    // Row 1, Size 4
    int tempInt2 = 10;
    // Row 2, Size 4
    int tempInt3 = 10;
    // Row 3, Size 4
    float tempFloat2 = 20.0f;
    // Row 4, Size 48
    Mat3x4 tempMat3x4 = {};
    // Row 5, Size 64
    Matrix tempMatrix = {};

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::IntType,
        FieldType::IntType,
        FieldType::FloatType,
        FieldType::Mat3x4Type,
        FieldType::Mat4Type,
    };
    static constexpr const char* fieldNames[] =
    {
        "tempInt",
        "tempInt2",
        "tempInt3",
        "tempFloat2",
        "tempMat3x4",
        "tempMatrix",
    };

    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json);

private:
    void* getElementIndexRef(u32 index);
    const void* getElementIndex(u32 index) const;
};

struct StaticModelEntity
{
    ~StaticModelEntity();
    struct StaticModelEntityReadWriteHandleBuilder
    {
        StaticModelEntityReadWriteHandleBuilder& addArrayRead(ComponentType componentType);
        StaticModelEntityReadWriteHandleBuilder& addArrayWrite(ComponentType componentType);

        u64 readArrays = 0;
        u64 writeArrays = 0;
    };

    struct StaticModelEntityEntityLockedMutexHandle
    {
        u64 lockIndex = 0;
    };

    static constexpr ComponentType componentTypes[] =
    {
        Heritaged1::componentID,
        Heritaged2::componentID,
    };

    static constexpr const char* entitySystemName = "StaticModelEntity";
    static constexpr EntityType entitySystemID = EntityType::StaticModelEntityType;
    static constexpr u32 entityVersion = 1;
    static constexpr u32 componentTypeCount = sizeof(componentTypes) / sizeof(ComponentType);

    static u32 getComponentIndex(ComponentType componentType);
    bool hasComponent(EntitySystemHandle handle, ComponentType componentType) const;

    // Different handle types for getting array... These are needed to set atomic locks...
    static constexpr StaticModelEntityReadWriteHandleBuilder getReadWriteHandleBuilder() { return StaticModelEntityReadWriteHandleBuilder(); }
    const EntityReadWriteHandle getReadWriteHandle(const StaticModelEntityReadWriteHandleBuilder& builder);

    StaticModelEntityEntityLockedMutexHandle getLockedMutexHandle();
    bool releaseLockedMutexHandle(const StaticModelEntityEntityLockedMutexHandle& handle);

    bool syncReadWrites();

    EntitySystemHandle getEntitySystemHandle(u32 index) const;
    EntitySystemHandle addEntity(const StaticModelEntityEntityLockedMutexHandle& handle);
    bool removeEntity(EntitySystemHandle handle, const StaticModelEntityEntityLockedMutexHandle &mutexHandle);

    const Heritaged1* getHeritaged1ReadArray(const EntityReadWriteHandle& handle) const;
    Heritaged1* getHeritaged1WriteArray(const EntityReadWriteHandle& handle);
    const Heritaged2* getHeritaged2ReadArray(const EntityReadWriteHandle& handle) const;
    Heritaged2* getHeritaged2WriteArray(const EntityReadWriteHandle& handle);

    bool addHeritaged1Component(EntitySystemHandle handle, const Heritaged1& component);
    bool addHeritaged2Component(EntitySystemHandle handle, const Heritaged2& component);
    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json, const StaticModelEntityEntityLockedMutexHandle &mutexHandle);
    u32 getEntityCount() const { return (u32)entityComponents.size(); }

private:
    std::vector<Heritaged1> Heritaged1Array;
    std::vector<Heritaged2> Heritaged2Array;

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(componentTypeCount < 64, "Only 64 components are allowed for entity!");

    std::mutex entityAddRemoveMutex {};
    u64 mutexLockIndex = 0;

    std::atomic<u64> readArrays {0};
    std::atomic<u64> writeArrays {0};
    u32 currentSyncIndex = 0;

    bool entitiesAdded = false;
    bool entitiesRemoved = false;
};

struct OtherTestEntity
{
    ~OtherTestEntity();
    struct OtherTestEntityReadWriteHandleBuilder
    {
        OtherTestEntityReadWriteHandleBuilder& addArrayRead(ComponentType componentType);
        OtherTestEntityReadWriteHandleBuilder& addArrayWrite(ComponentType componentType);

        u64 readArrays = 0;
        u64 writeArrays = 0;
    };

    struct OtherTestEntityEntityLockedMutexHandle
    {
        u64 lockIndex = 0;
    };

    static constexpr ComponentType componentTypes[] =
    {
        Heritaged1::componentID,
    };

    static constexpr const char* entitySystemName = "OtherTestEntity";
    static constexpr EntityType entitySystemID = EntityType::OtherTestEntityType;
    static constexpr u32 entityVersion = 1;
    static constexpr u32 componentTypeCount = sizeof(componentTypes) / sizeof(ComponentType);

    static u32 getComponentIndex(ComponentType componentType);
    bool hasComponent(EntitySystemHandle handle, ComponentType componentType) const;

    // Different handle types for getting array... These are needed to set atomic locks...
    static constexpr OtherTestEntityReadWriteHandleBuilder getReadWriteHandleBuilder() { return OtherTestEntityReadWriteHandleBuilder(); }
    const EntityReadWriteHandle getReadWriteHandle(const OtherTestEntityReadWriteHandleBuilder& builder);

    OtherTestEntityEntityLockedMutexHandle getLockedMutexHandle();
    bool releaseLockedMutexHandle(const OtherTestEntityEntityLockedMutexHandle& handle);

    bool syncReadWrites();

    EntitySystemHandle getEntitySystemHandle(u32 index) const;
    EntitySystemHandle addEntity(const OtherTestEntityEntityLockedMutexHandle& handle);
    bool removeEntity(EntitySystemHandle handle, const OtherTestEntityEntityLockedMutexHandle &mutexHandle);

    const Heritaged1* getHeritaged1ReadArray(const EntityReadWriteHandle& handle) const;
    Heritaged1* getHeritaged1WriteArray(const EntityReadWriteHandle& handle);

    bool addHeritaged1Component(EntitySystemHandle handle, const Heritaged1& component);
    bool serialize(WriteJson &json) const;
    bool deserialize(const JsonBlock &json, const OtherTestEntityEntityLockedMutexHandle &mutexHandle);
    u32 getEntityCount() const { return (u32)entityComponents.size(); }

private:
    std::vector<Heritaged1> Heritaged1Array;

    std::vector<u16> entityVersions;

    // This might be problematic if component is activated/deactived in middle of a frame
    std::vector<u64> entityComponents;

    // Need to think how this adding should work, because it would need to have mutex and all.
    std::vector<u32> freeEntityIndices;

    static_assert(componentTypeCount < 64, "Only 64 components are allowed for entity!");

    std::mutex entityAddRemoveMutex {};
    u64 mutexLockIndex = 0;

    std::atomic<u64> readArrays {0};
    std::atomic<u64> writeArrays {0};
    u32 currentSyncIndex = 0;

    bool entitiesAdded = false;
    bool entitiesRemoved = false;
};
