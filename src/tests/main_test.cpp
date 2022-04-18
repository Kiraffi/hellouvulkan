#include "container/bytebuffer.h"
#include "container/podvector.h"
#include "container/stackstring.h"
#include "core/mytypes.h"

struct Foos
{
    int i;
    int j;
};
#include <vector>
void testMemory(bool forceDefrag)
{
    ByteBuffer buff(8);
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

    defragMemory();
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

int main()
{
    testingMemory();
    testStackString();
    testMemoryStackString();
    return 0;
}