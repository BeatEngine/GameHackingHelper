# include "GameHackingHelper.h"

int main(int argc, char* argv[])
{
    printf("Test!\n");

    int * pid;
    HANDLE proc = attach("gedit", pid);

    printf("process: %d\n", proc);

    uintptr_t module = getmoduleaddress(proc, "gedit");

    printf("module: %"PRIxPTR"\n", module);

    ProcessAddress base(proc);
    base.load(proc, module);

    printf("Read:[%"PRIxPTR"]%d\n",base.getAddress(),base.read<int>());

    return 0;
}

