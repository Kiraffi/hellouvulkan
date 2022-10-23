#ifndef COMPONENTS_INCLUDE_FILE_H
#define COMPONENTS_INCLUDE_FILE_H

#include "components_macro.h"
#include "../generated_header.h"

SER_DATA_BEGIN(Heritaged, ComponentType::HeritagedType, 1)
    INT_FIELD(TempInt, 10)
    FLOAT_FIELD(TempFloat, 20.0f)
    VEC2_FIELD(TempV2, 41, 520)
    VEC3_FIELD(TempV3, 10, 20, 30)
    VEC4_FIELD(TempV4, 7, 8, -30, 10)
SER_DATA_END(Heritaged)


SER_DATA_BEGIN(Heritaged2, ComponentType::HeritagedType2, 1)
    INT_FIELD(TempInt2, 10)
    FLOAT_FIELD(TempFloat2, 20.0f)
SER_DATA_END(Heritaged2)


SER_DATA_BEGIN(Heritaged3, ComponentType::HeritagedType3, 1)
    INT_FIELD(TempInt3, 10)
    INT_FIELD(TempInt4, 20)
    FLOAT_FIELD(TempFloat3, 20.0f)
SER_DATA_END(Heritaged3)


/*
class EntityTypeSystem
{
public:
    static constexpr const* entityTypeName = "EntityName";
    static constexpr unsigned int entityTypeID = EntityType::EntityTypeId;

    struct EntityNameTypeEntityHandle { int index; int iterationNumber; }
    bool serialize();
    bool deserialize();

    bool getHandleForIndex(int index, EntityNameTypeEntityHandle &outHandle) const;

    // Check that the accessor only has either read or write, cannot have both, and
    // definitely no adding...
    const std::Vector<Type>& getTypeVectorRead() const;

    // Only one writer, no readers until sync point.
    PodVector<Type>& getTypeVectorWrite();

    const PodVector<u8>& getEntityStateVector() const;

    // Check accessors... cannot have read nor write accessors at all or even getActive...
    // Needs some locking mechanism...
    bool startAddingEntities();
    EntityNameTypeEntityHandle addEntity();
    bool endAddingEntities();

    // Making entity inactive should be fine.
    bool removeEntity(unsigned int index);

    // Called on sync points.
    void resetAccessors();

private:
    PodVector<u64> activeComponents;
    PodVector<Type> ;
    PodVector<Type2>;

    std::atomic<u32> accessorsForType1; // some bits, read, write
    std::atomic<u32> accessorsForType2; // some bits, read, write


    std::atomic<u32> accessorAddActive; // accessors for adding or getEntityStateVector being called.

    // These probably need mutexes especially when adding entities....
    PodVector<u16> entityVersions; // what versionNumber is each entity at?
    PodVector<u32> unusedEntities; // Where is a free slot at? Could be bad idea, maybe scanning is better?
                                   // thinking about multithreading, this would require lock, where as scanning
                                   // would just work, although it might be expensive to scan to find holes.
                                   // These adds would still probably be deferred to some place... have to think
}
*/
#endif