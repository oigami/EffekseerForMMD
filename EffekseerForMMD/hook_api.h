#pragma once
#include <string>
#include <unordered_map>
#include <bemapiset.h>
#include <winbase.h>
#include <wincon.h>

#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;

void* RewriteFunction(const char* szRewriteModuleName, const char* szRewriteFunctionName, void* pRewriteFunctionPointer, int ordinal = -1);
void modifyIAT(char* modname, void* origaddr, void* newaddr);
HMODULE dllModule();

namespace efk
{
#define HOOK_CREATE_FUNC(dllname, result, name, ...) using F##name= result (WINAPI*) (__VA_ARGS__);\
                                            F##name PF##name;              \
                                            result WINAPI my##name (__VA_ARGS__); namespace hook_rewrite { \
                                              void Rewrite##name(){printf("LoadLibrary %s:%p\n", dllname ,LoadLibrary(dllname ".dll")); PF##name= (F##name) RewriteFunction(dllname, #name, my##name);}\
                                              void Restore##name(){printf("LoadLibrary %s:%p\n", dllname ,LoadLibrary(dllname ".dll")); PF##name= (F##name) RewriteFunction(dllname, #name, PF##name);}\
                                            }\
                                            result WINAPI my##name (__VA_ARGS__)
#define HOOK_KERNEL32_CREATE_FUNC(result, name, ...) HOOK_CREATE_FUNC("Kernel32", result, name, __VA_ARGS__)

  namespace
  {
    bool nowEFKLoading = false;

    struct ReadFileData
    {
      ReadFileData(const std::string& name) : filename(name)
      {
        filename.resize(20);
      }

      std::string filename;
      int read_cnt = 0;
      constexpr static int filename_pos = sizeof(char[3]) + sizeof(float);
      constexpr static int filename_pos_end = sizeof(char[3]) + sizeof(float) + 20;
    };

    std::unordered_map<HANDLE, ReadFileData> now_read_pmd_file;

    HOOK_KERNEL32_CREATE_FUNC(BOOL, CloseHandle, HANDLE hObject)
    {
      now_read_pmd_file.erase(hObject);
      printf("CloseHandle: %p\n", hObject);
      return PFCloseHandle(hObject);
    }


    HOOK_KERNEL32_CREATE_FUNC(HANDLE, CreateFileA, LPCSTR lpFileName,
      DWORD dwDesiredAccess,
      DWORD dwShareMode,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      DWORD dwCreationDisposition,
      DWORD dwFlagsAndAttributes,
      HANDLE hTemplateFile)
    {
      puts(lpFileName);
      return PFCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    HOOK_KERNEL32_CREATE_FUNC(HANDLE, CreateFileW, LPCWSTR lpFileName,
      DWORD dwDesiredAccess,
      DWORD dwShareMode,
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      DWORD dwCreationDisposition,
      DWORD dwFlagsAndAttributes,
      HANDLE hTemplateFile)
    {
      bool is_default_pmd = false;
      filesystem::path path(lpFileName);
      const auto efk_filename = path.filename().stem().stem().string();
      if ( !nowEFKLoading && path.extension() == L".efk" )
      {
        char module_path[MAX_PATH + 1];
        path = path.parent_path() / path.stem().stem().replace_extension(".pmd");

        // efkファイルの場所に同名のpmdファイルが有る場合そのファイルを優先
        if ( filesystem::exists(path) )
        {
          lpFileName = path.c_str();
        }
        else if ( 0 != GetModuleFileNameA(dllModule(), module_path, MAX_PATH) )
        {
          // dllの場所にあるファイルを使用
          path = filesystem::path(module_path).parent_path() / L"efk.pmd";
          if ( filesystem::exists(path) == false )
          {
            MessageBoxW(nullptr, L"EffekseerForMMDの動作に必要なefk.pmdがありません。\nもう一度インストールを試してください", L"エラー", MB_OK);
          }
          lpFileName = path.c_str();
          is_default_pmd = true;
        }
      }
      auto handle = PFCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
      printf("myCreateFile : %p %d\n", handle, GetLastError());
      _putws(lpFileName);
      if ( is_default_pmd )
      {
        now_read_pmd_file.insert({ handle, efk_filename });
      }
      return handle;
    }

    HOOK_KERNEL32_CREATE_FUNC(BOOL, ReadFile,
      HANDLE hFile, // ファイルのハンドル
      LPVOID lpBuffer, // データバッファ
      DWORD nNumberOfBytesToRead, // 読み取り対象のバイト数
      LPDWORD lpNumberOfBytesRead, // 読み取ったバイト数
      LPOVERLAPPED lpOverlapped) // オーバーラップ構造体のバッファ
    {
      //printf("ReadFile %d : %d\n", hFile, nNumberOfBytesToRead);
      auto res = PFReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
      auto it = now_read_pmd_file.find(hFile);
      if ( it != now_read_pmd_file.end() )
      {
        auto& data = it->second;
        if ( data.filename_pos <= data.read_cnt && data.read_cnt < data.filename_pos_end )
        {
          int begin_pos = data.read_cnt - data.filename_pos;
          memcpy(static_cast<BYTE*>(lpBuffer) + begin_pos, data.filename.c_str() + begin_pos, *lpNumberOfBytesRead - begin_pos);
        }
        data.read_cnt += *lpNumberOfBytesRead;
      }
      return res;
    }

    HOOK_KERNEL32_CREATE_FUNC(DWORD, SetFilePointer,
      HANDLE hFile, // ファイルのハンドル
      LONG lDistanceToMove, // ポインタを移動するべきバイト数
      PLONG lpDistanceToMoveHigh, // ポインタを移動するべきバイト数
      DWORD dwMoveMethod) // 開始点
    {
      printf("setpointer: %p %d  ,%d\n", lpDistanceToMoveHigh, lDistanceToMove, dwMoveMethod);
      return PFSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
    }


    HOOK_CREATE_FUNC("Shell32",
      UINT,
      DragQueryFileW,
      HDROP hDrop, // ファイル名構造体のハンドル
      UINT iFile, // ファイルのインデックス番号
      LPWSTR lpszFile, // ファイル名を格納するバッファ
      UINT cch) // バッファのサイズ
    {
      auto res = PFDragQueryFileW(hDrop, iFile, lpszFile, cch);
      filesystem::path path(lpszFile);
      if ( path.extension() == L".efk" )
      {
        const int add_len = sizeof(L".pmd.efk") / sizeof(wchar_t) - 1;
        if ( res + add_len >= cch )
        {
          using namespace std::string_literals;
          std::string error_msg = "ファイルパスが長過ぎます。\nディレクトリ名やファイル名を短くするか、階層を少なくしてください\n\nファイルパス:\n"s + path.string();

          MessageBoxA(nullptr, error_msg.c_str(), "Effekseer on MMD", MB_OK | MB_ICONWARNING);
        }
        else
        {
          lstrcatW(lpszFile, L".pmd.efk");
          res += add_len;
        }
      }

      return res;
    }

    HOOK_CREATE_FUNC("Shell32",
      VOID,
      DragFinish,
      HDROP hDrop)
    {
      return PFDragFinish(hDrop);
    }
  }
}
