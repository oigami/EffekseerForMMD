#pragma once
#include <string>
#include <unordered_map>
#include <winbase.h>

#include <mutex>
#include <experimental/filesystem>
#include <io.h>
namespace filesystem = std::experimental::filesystem;

HMODULE dllModule();

namespace efk
{
#define HOOK_CREATE_FUNC(dllname, result, name, ...) mmp::WinAPIHooker<decltype(name)*> PF##name;\
                                            result WINAPI my##name (__VA_ARGS__);\
                                            void Rewrite##name(){ PF##name.hook(dllname ".dll",#name, &my##name);}\
                                            result WINAPI my##name (__VA_ARGS__)
#define HOOK_KERNEL32_CREATE_FUNC(result, name, ...) HOOK_CREATE_FUNC("Kernel32", result, name, __VA_ARGS__)

  namespace hook_rewrite
  {
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

      std::unordered_map<int, ReadFileData> now_read_pmd_file;
      std::mutex now_read_pmd_file_mutex;

      HOOK_CREATE_FUNC("MSVCR90", errno_t, _wsopen_s, int* pfh, const wchar_t* filename, int oflag, int shflag, int pmode)
      {
        bool is_default_pmd = false;
        filesystem::path path(filename);
        const auto efk_filename = path.filename().stem().stem().string();
        if ( !nowEFKLoading && path.extension() == L".efk" )
        {
          char module_path[MAX_PATH + 1];
          path = path.parent_path() / path.stem().stem().replace_extension(".pmd");

          // efkファイルの場所に同名のpmdファイルが有る場合そのファイルを優先
          if ( filesystem::exists(path) )
          {
            filename = path.c_str();
          }
          else if ( 0 != GetModuleFileNameA(dllModule(), module_path, MAX_PATH) )
          {
            // dllの場所にあるファイルを使用
            path = filesystem::path(module_path).parent_path() / L"efk.pmd";
            if ( filesystem::exists(path) == false )
            {
              MessageBoxW(nullptr, L"EffekseerForMMDの動作に必要なefk.pmdがありません。\nもう一度インストールを試してください", L"エラー", MB_OK);
            }
            filename = path.c_str();
            is_default_pmd = true;
          }
        }
        auto handle = PF_wsopen_s(pfh, filename, oflag, shflag, pmode);
        _putws(filename);
        if ( is_default_pmd )
        {
          std::lock_guard<std::mutex> lock(now_read_pmd_file_mutex);
          now_read_pmd_file.insert({ *pfh, efk_filename });
        }
        return handle;
      }

      HOOK_CREATE_FUNC("MSVCR90", int, _read, int pfh, void* dstBuf, unsigned int maxCharCount)
      {
        auto res = PF_read(pfh, dstBuf, maxCharCount);
        std::lock_guard<std::mutex> lock(now_read_pmd_file_mutex);
        auto it = now_read_pmd_file.find(pfh);
        if ( it != now_read_pmd_file.end() )
        {
          auto& data = it->second;
          if ( data.filename_pos <= data.read_cnt && data.read_cnt < data.filename_pos_end )
          {
            int begin_pos = data.read_cnt - data.filename_pos;
            memcpy(static_cast<BYTE*>(dstBuf) + begin_pos, data.filename.c_str() + begin_pos, res - begin_pos);
          }
          data.read_cnt += res;
        }
        return res;
      }

      HOOK_CREATE_FUNC("MSVCR90", errno_t, _close, int pfh)
      {
        std::lock_guard<std::mutex> lock(now_read_pmd_file_mutex);
        now_read_pmd_file.erase(pfh);
        return PF_close(pfh);
      }

      HOOK_KERNEL32_CREATE_FUNC(DWORD, SetFilePointer,
                                HANDLE hFile, // ファイルのハンドル
                                LONG lDistanceToMove, // ポインタを移動するべきバイト数
                                PLONG lpDistanceToMoveHigh, // ポインタを移動するべきバイト数
                                DWORD dwMoveMethod) // 開始点
      {
        //printf("setpointer: %p %d  ,%d\n", lpDistanceToMoveHigh, lDistanceToMove, dwMoveMethod);
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
        if ( path.extension() == L".efkproj" )
        {
          MessageBoxW(nullptr, L"Effekseer for MMDで読み込める拡張子は「.efk」のものです。Effekseerのツールを使用して「.efk」を出力してください", L"エラー", MB_OK);
        }
        else if ( path.extension() == L".efk" )
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
}
