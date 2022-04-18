#include "container/bytebuffer.h"
#include "container/podvector.h"
#include "core/mytypes.h"

struct Foos
{
    int i;
    int j;
};

int main()
{
    ByteBuffer buff(8);
    {
        ByteBuffer buff2(8);
        buff2.reserve(40);
        buff.reserve(40);
    }
    PodVector<Foos> foos;
    for(int i = 0; i < 10; ++i)
    {
        foos.pushBack(Foos{i, i + 1});

    }
    int counter = 0;
    for(Foos &f : foos)
    {
        printf("counter: %i: i: %i, j: %i\n", counter, f.i, f.j);
        ++counter;
    }

    defragMemory();
}