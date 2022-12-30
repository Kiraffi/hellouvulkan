#include <core/assert.h>

template <typename T>
class Nullable
{
public:
    ~Nullable()
    {
        ASSERT(value == nullptr);
        delete value;
        value = nullptr;
    }
    void create()
    {
        ASSERT(value == nullptr);
        value = new T();
    }
    void destroy()
    {
        ASSERT(value);
        delete value;
        value = nullptr;
    }
    T* get() const
    {
        ASSERT(value);
        return value;
    }
private:
    T* value = nullptr;
};