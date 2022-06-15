#define COMPONENT(name_str, magicnumber, versionnumber) struct name_str \
{ \
    static constexpr uint32_t magicNumber = magicnumber; \
    static constexpr uint32_t versionNumber = versionnumber;

#define ADD_


#define END_COMPONENT() }