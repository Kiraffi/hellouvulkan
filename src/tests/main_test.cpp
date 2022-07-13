#include "testfuncs.h"

#include <container/arraysliceview.h>
#include <container/bytebuffer.h>
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/stackstring.h>
#include <container/vector.h>

#include <core/mytypes.h>

#include <myvulkan/uniformbuffermanager.h>

struct Foos
{
    int i;
    int j;
};
void testMemory(bool forceDefrag)
{
    ByteBuffer buff(8, BufferType::UNKNOWN);
    buff.reserve(40);
    PodVector<Foos> foos{ Foos{1000, 1000}, Foos{1002, 1200}} ;

    for(int i = 0; i < 30; ++i)
    {
        foos.pushBack(Foos{i, i + 1});
    }

    if(forceDefrag)
        buff.reserve(1140);

    for(int i = 20; i < 30; ++i)
    {
        foos.pushBack(Foos{i, i + 1});
    }

    for(int i = 0; i < 10; ++i)
    {
        foos.insertIndex(i * 2, Foos{i + 100, i + 100 + 1});
    }

    int counter = 0;
    for(Foos &f : foos)
    {
        printf("counter: %i: i: %i, j: %i\n", counter, f.i, f.j);
        ++counter;
    }

    PodVector<Foos> faas = foos;
    foos.removeIndex(0);
    counter = 0;
    for(Foos &f : faas)
    {
        printf("counter-copy: %i: i: %i, j: %i\n", counter, f.i, f.j);
        ++counter;
    }


    defragMemory();
    foos.clear();
}

void testStackString()
{
    SmallStackString str = "This should be string!";
    SmallStackString str2 = "This should be string that will not fit!";
    printf("%s\n", str.getStr());
    printf("%s\n", str2.getStr());

    printf("%c\n", str[2]);
}

void testingMemory()
{
    // with false it should not defrag memory, because they are all working like stack.
    testMemory(false);
    printf("\n\n\n");
    // with true it should defrag memory, because memory is being pushed in middle in linear allocator.
    testMemory(true);
}

void testMemoryStackString()
{
    PodVector<SmallStackString> strings;
    strings.pushBack("This should be first string!");
    strings.pushBack("This should be second string!");
    SmallStackString s = "This should be: ";
    for(int i = 0; i < 100; ++i)
    {
        SmallStackString s2 = s;
        s2.add(i + 0.5f);
        s2.add(" string!");
        strings.pushBack(s2);
    }
    for(const SmallStackString &str : strings)
    {
        printf("%s\n", str.getStr());
    }
}

void testUniformBufferManager()
{
    UniformBufferManager manager;
    // Bit illegal but...
    manager.init((Buffer &)manager);

    PodVector<UniformBufferHandle> handles;

    for (uint32_t i = 0; i < 100; ++i)
    {
        handles.pushBack(manager.reserveHandle());
    }
    manager.freeHandle(handles[70]);
    // will assert
    //manager.freeHandle(handles[70]);

    handles.removeIndex(70);

    for (uint32_t i = 0; i < handles.size(); ++i)
    {
        printf("%i: Handle: %u\n", i, handles[i].index);
    }
    handles.pushBack(manager.reserveHandle());
}

struct TempStr
{
    int foo = 0;
    int faa = 1;

};
void printSlice(const ArraySliceView<TempStr>& slice)
{
    printf("slice ptr: %p\n", slice.data());
    printf("slice size: %u\n", slice.size());

}

void testArraySliceView()
{
    PodVector<uint32_t> uints;
    for(uint32_t i = 0; i < 32; ++i)
        uints.pushBack(i);

    auto slice = sliceFromPodVector(uints);
    for (auto v : slice)
        printf("value: %u\n", v);
}

void printUints(const PodVector<uint32_t> &vec)
{
    for(const auto v : vec)
        printf("%u, ", v);
    printf("\n");
}

void printvector(const Vector<PodVector<uint32_t>> &vectorOfPodVectors)
{
    for(const auto &vec : vectorOfPodVectors)
    {
        printUints(vec);
    }
    printf("\n");
}
void testVector()
{
    Vector<PodVector<uint32_t>> vectorOfPodVectors{{1}, {2, 99, 43, 3}};

    vectorOfPodVectors.pushBack({1, 2, 3});

    vectorOfPodVectors.pushBack({4, 5, 60});
    vectorOfPodVectors.pushBack({7, 8, 9});
    printvector(vectorOfPodVectors);

    vectorOfPodVectors.insertIndex(1, {4, 4, 4, 1, 2, 3});
    printvector(vectorOfPodVectors);

    vectorOfPodVectors.removeIndex(2);
    printvector(vectorOfPodVectors);
    vectorOfPodVectors.removeIndex(3);
    printvector(vectorOfPodVectors);
    vectorOfPodVectors.removeIndex(0);
    printvector(vectorOfPodVectors);

    for(uint32_t i = 0; i < vectorOfPodVectors.getSize(); ++i)
        printUints(vectorOfPodVectors[i]);


    Vector<PodVector<uint32_t>> v = vectorOfPodVectors;
    printvector(v);
    vectorOfPodVectors.removeIndex(1);
    v.removeIndex(0);
    printvector(vectorOfPodVectors);
    printvector(v);

    Vector<Vector<PodVector<uint32_t>>> v2;
    v2.resize(8);
    for(uint32_t i = 0; i < 3; ++i)
        v2.push_back(vectorOfPodVectors);

    vectorOfPodVectors.clear();

    for(const auto &vec : v2)
        printvector(vec);


    printvector(vectorOfPodVectors);

}

int main()
{
    initMemory();
    testingMemory();
    testStackString();
    testMemoryStackString();
    testUniformBufferManager();
    testArraySliceView();

    testVector();

    testMatrix();
    testMathVector();

    testStrings();
    deinitMemory();
    return 0;
}