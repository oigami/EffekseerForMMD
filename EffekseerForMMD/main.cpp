#include <type_traits>
#include <vector>

#include <Windows.h>
#include <stdio.h>
#include <assert.h>

void OpenConsole()
{
  FILE *in, *out;
  AllocConsole();
  freopen_s(&out, "CONOUT$", "w", stdout);
  freopen_s(&in, "CONIN$", "r", stdin);
}
static HMODULE module;
HMODULE dllModule()
{
  return module;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  module = hModule;
  switch ( ul_reason_for_call )
  {
  case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
    OpenConsole();
#endif // !NDEBUG
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
