#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <conio.h>
#include <shellapi.h>

#include <unordered_map>
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#include <effekseer/include/Effekseer.h>
#include <effekseer/include/EffekseerRendererDX9.h>

#include "effekseer_dll.h"

#ifndef NDEBUG
# pragma comment(lib,"effekseer/lib/VS2015/Debug/x64/EffekseerRendererDX9")
# pragma comment(lib,"effekseer/lib/VS2015/Debug/x64/Effekseer")
#else
# pragma comment(lib,"effekseer/lib/VS2015/Release/x64/EffekseerRendererDX9")
# pragma comment(lib,"effekseer/lib/VS2015/Release/x64/Effekseer")

# define printf(...) (void)0
# define puts(...) (void)0
# define _putws(...) (void)0
#endif // NDEBUG

#include <mmd/MMDExport.h>

void* RewriteFunction(const char* szRewriteModuleName, const char* szRewriteFunctionName, void* pRewriteFunctionPointer, int ordinal = -1);
void modifyIAT(char* modname, void* origaddr, void* newaddr);
HMODULE dllModule();

namespace efk
{
  namespace
  {
    bool nowEFKLoading = false;

    Effekseer::Matrix44 toMatrix4x4(const D3DMATRIX& mat)
    {
      Effekseer::Matrix44 ans;
      for ( int i = 0; i < 4; i++ )
      {
        for ( int j = 0; j < 4; j++ )
        {
          ans.Values[i][j] = mat.m[i][j];
        }
      }
      return ans;
    }

    Effekseer::Matrix43 toMatrix4x3(const D3DMATRIX& mat)
    {
      Effekseer::Matrix43 ans;
      for ( int i = 0; i < 4; i++ )
      {
        for ( int j = 0; j < 3; j++ )
        {
          ans.Value[i][j] = mat.m[i][j];
        }
      }
      return ans;
    }
  }


  D3D9DeviceEffekserr::D3D9DeviceEffekserr(IDirect3DDevice9* device) : now_present(false), device(device)
  {
    HookAPI();
    g_renderer = ::EffekseerRendererDX9::Renderer::Create(device, 10000);

    // エフェクト管理用インスタンスの生成
    g_manager = ::Effekseer::Manager::Create(10000);

    // 描画用インスタンスから描画機能を設定
    g_manager->SetSpriteRenderer(g_renderer->CreateSpriteRenderer());
    g_manager->SetRibbonRenderer(g_renderer->CreateRibbonRenderer());
    g_manager->SetRingRenderer(g_renderer->CreateRingRenderer());
    g_manager->SetTrackRenderer(g_renderer->CreateTrackRenderer());
    g_manager->SetModelRenderer(g_renderer->CreateModelRenderer());

    // 描画用インスタンスからテクスチャの読込機能を設定
    // 独自拡張可能、現在はファイルから読み込んでいる。
    g_manager->SetTextureLoader(g_renderer->CreateTextureLoader());
    g_manager->SetModelLoader(g_renderer->CreateModelLoader());

    // 音再生用インスタンスの生成
    //g_sound = ::EffekseerSound::Sound::Create(g_xa2, 16, 16);

    // 音再生用インスタンスから再生機能を指定
    //g_manager->SetSoundPlayer(g_sound->CreateSoundPlayer());

    // 音再生用インスタンスからサウンドデータの読込機能を設定
    // 独自拡張可能、現在はファイルから読み込んでいる。
    //g_manager->SetSoundLoader(g_sound->CreateSoundLoader());

    // 投影行列を設定
    g_renderer->SetProjectionMatrix(
      ::Effekseer::Matrix44().PerspectiveFovRH(90.0f / 180.0f * 3.14f, 1024.0f / 768.0f, 1.0f, 500000.0f));
  }

  void fps()
  {
    int i;
    static int t = 0, ave = 0, f[60];
    static int count = 0;
    count++;
    f[count % 60] = GetTickCount() - t;
    t = GetTickCount();
    if ( count % 60 == 59 )
    {
      ave = 0;
      for ( i = 0; i < 60; i++ ) ave += f[i];
      ave /= 60;
    }
    if ( ave != 0 )
    {
      printf("%.1fFPS \t", 1000.0 / (double) ave);
      printf("%dms", ave);
    }
    return;
  }

  void D3D9DeviceEffekserr::DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
  {
    const int pmd_num = ExpGetPmdNum();
    const int technic_type = ExpGetCurrentTechnic();
    const int now_render_object = ExpGetCurrentObject();
    const int now_render_material = ExpGetCurrentMaterial();
    UpdateProjection();
    if ( D3DPT_LINELIST != Type && now_render_material == 0 && now_render_object != 0
      && !now_present && (technic_type == 1 || technic_type == 2) )
      for ( int i = 0; i < pmd_num; i++ )
      {
        if ( now_render_object != ExpGetPmdOrder(i) ) continue;

        UpdateCamera();
        const int ID = ExpGetPmdID(i);
        auto it = effect.find(ID);
        if ( it != effect.end() )
        {
          auto center = ExpGetPmdBoneWorldMat(i, 0);
          auto play_mat = ExpGetPmdBoneWorldMat(i, 1);
          double play_time = 0.0;
          play_time += pow(static_cast<double>(center.m[3][0]) - play_mat.m[3][0], 2);
          play_time += pow(static_cast<double>(center.m[3][1]) - play_mat.m[3][1], 2);
          play_time += pow(static_cast<double>(center.m[3][2]) - play_mat.m[3][2], 2);
          play_time = sqrt(play_time) - 0.5;

          it->second.setMatrix(center);
          it->second.update(static_cast<float>(play_time));

          now_present = true;

          // Effekseerライブラリがリストアしてくれないので自前でバックアップしてる
          float constant_data[256 * 4];
          device->GetVertexShaderConstantF(0, constant_data, sizeof(constant_data) / sizeof(float) / 4);

          UINT stride;
          IDirect3DVertexBuffer9* stream_data;
          UINT offset;
          device->GetStreamSource(0, &stream_data, &offset, &stride);

          IDirect3DIndexBuffer9* index_data;
          device->GetIndices(&index_data);

          // エフェクトの描画開始処理を行う。
          if ( g_renderer->BeginRendering() )
          {
            // エフェクトの描画を行う。
            it->second.draw();

            // エフェクトの描画終了処理を行う。
            g_renderer->EndRendering();
          }

          device->SetStreamSource(0, stream_data, offset, stride);
          if ( stream_data ) stream_data->Release();

          device->SetIndices(index_data);
          if ( index_data ) index_data->Release();

          device->SetVertexShaderConstantF(0, constant_data, sizeof(constant_data) / sizeof(float) / 4);

          now_present = false;
        }
      }
  }

  void D3D9DeviceEffekserr::BeginScene(void)
  {
    int len = len = ExpGetPmdNum();
    if ( len != effect.size() )
      for ( int i = 0; i < len; i++ )
      {
        const int id = ExpGetPmdID(i);
        const auto file_name = ExpGetPmdFilename(i);
        filesystem::path path(file_name);
        if ( ".efk" == path.extension() )
        {
          auto it = effect.insert({ id, MyEffect() });
          if ( !it.second ) continue;

          // エフェクトの読込
          nowEFKLoading = true;
          auto eff = Effekseer::Effect::Create(g_manager, reinterpret_cast<const EFK_CHAR*>((path.remove_filename() / path.stem().stem()).c_str()));
          nowEFKLoading = false;
          it.first->second = MyEffect(g_manager, eff);
        }
      }
  }

  void D3D9DeviceEffekserr::EndScene(void)
  {
    //g_renderer->SetCameraMatrix(Effekseer::Matrix44().Indentity());
    //UpdateCamera();

    // エフェクトの移動処理を行う
    //g_manager->AddLocation(g_handle, ::Effekseer::Vector3D(0.2f, 0.0f, 0.0f));

    // エフェクトの更新処理を行う

    printf("%f\n", ExpGetFrameTime());
  }


  void D3D9DeviceEffekserr::UpdateCamera() const
  {
    D3DMATRIX view, world;
    device->GetTransform(D3DTS_WORLD, &world);
    device->GetTransform(D3DTS_VIEW, &view);
    Effekseer::Matrix44 o, eview, eworld;
    for ( int i = 0; i < 4; i++ )
    {
      for ( int j = 0; j < 4; j++ )
      {
        eview.Values[i][j] = view.m[i][j];
        eworld.Values[i][j] = world.m[i][j];
      }
    }
    //eworld.Values[3][2] = eview.Values[3][2];
    Effekseer::Matrix44::Mul(o, eworld, eview);

    auto toVec = [](float (&a)[4])
      {
        return Effekseer::Vector3D(a[0], a[1], a[2]);
      };
    g_renderer->SetCameraMatrix(o);
  }

  void D3D9DeviceEffekserr::UpdateProjection() const
  {
    D3DMATRIX projection;
    device->GetTransform(D3DTS_PROJECTION, &projection);
    Effekseer::Matrix44 mat;
    for ( int i = 0; i < 4; i++ )
    {
      for ( int j = 0; j < 4; j++ )
      {
        mat.Values[i][j] = projection.m[i][j];
      }
    }

    g_renderer->SetProjectionMatrix(mat);
  }

  MyEffect::MyEffect() : now_frame(0), manager(nullptr), handle(-1), effect(nullptr) { }

  MyEffect::MyEffect(Effekseer::Manager* manager, Effekseer::Effect* effect) : manager(manager), effect(effect)
  {
    create();
  }

  MyEffect::~MyEffect() { }

  void MyEffect::setMatrix(const D3DMATRIX& mat) const
  {
    auto mat4x3 = toMatrix4x3(mat);
    manager->BeginUpdate();
    manager->SetBaseMatrix(handle, mat4x3);
    manager->EndUpdate();
  }

  void MyEffect::update(float new_frame)
  {
    constexpr double eps = 1e-7;
    if ( now_frame > new_frame + eps )
    {
      manager->StopEffect(handle);
      now_frame = new_frame;
      return;
    }
    else ifCreate();
#if 1
    const int len = std::min(static_cast<int>(1e9), static_cast<int>(new_frame - now_frame));
    for ( int i = 0; i < len; i++ )
    {
      manager->BeginUpdate();
      manager->UpdateHandle(handle);
      manager->EndUpdate();
    }
    //manager->UpdateHandle(handle, new_frame - now_frame - len);
    now_frame += len;
    if ( len == 0 )
    {
      manager->BeginUpdate();
      manager->UpdateHandle(handle, 0.0f);
      manager->EndUpdate();
    }
#else
    manager->BeginUpdate();
    manager->UpdateHandle(handle, new_frame - now_frame);
    manager->EndUpdate();
    now_frame = new_frame;
#endif
  }

  void MyEffect::draw() const
  {
    manager->DrawHandle(handle);
  }

  void MyEffect::ifCreate()
  {
    if ( !manager->Exists(handle) ) create();
  }

  void MyEffect::create()
  {
    if ( manager->Exists(handle) ) manager->StopEffect(handle);
    handle = manager->Play(effect, 0.0f, 0.0f, 0.0f);
    manager->Flip();
    now_frame = 0.0;
    printf("create %f\n", ExpGetFrameTime());
  }

#define HOOK_CREATE_FUNC(dllname, result, name, ...) using F##name= result (*) (__VA_ARGS__);\
                                          F##name PF##name;              \
                                          result my##name (__VA_ARGS__); namespace hook_rewrite { void Rewrite##name(){printf("LoadLibrary %s:%d\n", dllname ,LoadLibrary(dllname ".dll")); PF##name= (F##name) RewriteFunction(dllname, #name, my##name); }}\
                                          result my##name (__VA_ARGS__)
#define HOOK_KERNEL32_CREATE_FUNC(result, name, ...) HOOK_CREATE_FUNC("Kernel32", result, name, __VA_ARGS__)

  namespace
  {
    struct ReadFileData
    {
      ReadFileData(const std::string& name): filename(name)
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
      printf("CloseHandle: %d\n", hObject);
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
          lpFileName = path.c_str();
          is_default_pmd = true;
        }
      }
      auto handle = PFCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
      printf("myCreateFile : %d %d\n", handle, GetLastError());
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
      printf("setpointer: %d %d  ,%d\n", lpDistanceToMoveHigh, lDistanceToMove, dwMoveMethod);
      return PFSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
    }


    HOOK_CREATE_FUNC("Shell32", UINT, DragQueryFileW,
      HDROP hDrop, // ファイル名構造体のハンドル
      UINT iFile, // ファイルのインデックス番号
      LPWSTR lpszFile, // ファイル名を格納するバッファ
      UINT cch // バッファのサイズ
    )
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

    HOOK_CREATE_FUNC("Shell32", VOID, DragFinish,
      HDROP hDrop)
    {
      return PFDragFinish(hDrop);
    }
  }

  void D3D9DeviceEffekserr::HookAPI()
  {
    hook_rewrite::RewriteCreateFileW();
    hook_rewrite::RewriteCloseHandle();
    hook_rewrite::RewriteReadFile();
    hook_rewrite::RewriteSetFilePointer();
    //hook_rewrite::RewriteDragFinish();
    HMODULE handle = LoadLibrary("shell32.dll");
    PFDragQueryFileW = reinterpret_cast<FDragQueryFileW>(GetProcAddress(handle, "DragQueryFileW"));
    modifyIAT("shell32.dll", PFDragQueryFileW, myDragQueryFileW);

    PFDragFinish = reinterpret_cast<FDragFinish>(GetProcAddress(handle, "DragFinish"));
    modifyIAT("shell32.dll", PFDragFinish, myDragFinish);

    // メニューバーの追加
    auto hwnd = getHWND();
    auto hmenu = GetMenu(hwnd);
    AppendMenuA(hmenu, MF_RIGHTJUSTIFY | MFS_GRAYED | MFS_DISABLED, 10000001, "Effekseer");
    DrawMenuBar(hwnd);
  }
}

int version() { return 1; }
MMDPluginDLL1* create1(IDirect3DDevice9* device) { return new efk::D3D9DeviceEffekserr(device); }
void destroy1(MMDPluginDLL1* p) { return delete p; }
