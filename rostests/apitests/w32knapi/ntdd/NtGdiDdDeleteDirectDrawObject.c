#include "../w32knapi.h"

W32KAPI
BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
)
{
	return (BOOL)Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDrawLocal);
}

INT
Test_NtGdiDdDeleteDirectDrawObject(PTESTINFO pti)
{
    TEST(NtGdiDdDeleteDirectDrawObject(NULL) == 0);

    return APISTATUS_NORMAL;
}
