#pragma once

namespace std
{
    template <typename T>
    class initializer_list;
}


template <typename T>
class Vector : public VectorBase
{
public:
    Vector();
    virtual ~Vector();
    //Vector(uint32_t size);

    Vector(const T *b, const T* e);
    Vector(const Vector<T> &vec);
    Vector(Vector<T> &&other) noexcept;

    Vector(const std::initializer_list<T> &initializerList);

    Vector& operator=(const Vector<T> &vec);

    T &operator[] (uint32_t index) const;

    void pushBack(const T &obj);
    void push_back(const T &obj) { pushBack(obj); }
    void emplace_back(const T &obj) { pushBack(obj); }
    void insertIndex(uint32_t index, const T &obj);
    void removeIndex(uint32_t index);

    void resize(uint32_t newSize);
    void resize(uint32_t newSize, const T &defaultValue);

    void clear();

    T *const begin() const { return (T *const)(getBegin()); }
    T *const data() const  { return (T *const)(getBegin()); }
    T *const end() const   { return (T *const)(getEnd()  ); }

    T& front() const { return *((T*)(getBegin())); }
    T& back()  const { return *((T*)(getBack() )); }

};
