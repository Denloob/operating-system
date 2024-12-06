#include "test.h"

typedef void (*TestFunction)();
static TestFunction test_funcs[] = {

};

void test_perform_all()
{
    for (int i = 0; i < (sizeof(test_funcs) / sizeof(*test_funcs)); i++)
    {
        test_funcs[i]();
    }
}
