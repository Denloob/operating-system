#include "test.h"

// Includes for the test functions
#include "kmalloc.h"
#include "test_filesystem.h"

typedef void (*TestFunction)();
static TestFunction test_funcs[] = {
    test_kmalloc,
    test_filesystem,
};

void test_perform_all()
{
    for (int i = 0; i < (sizeof(test_funcs) / sizeof(*test_funcs)); i++)
    {
        test_funcs[i]();
    }
}
