#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <wrl/client.h>

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
#include "hook_api.h"

namespace efk
{
  namespace
  {
    constexpr auto eps = 1e-7f;

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

  PMDResource::PMDResource(int i)
  {
    if ( i == -1 ) return;

    // モーフのIDを取得
    morph_id_.fill(0);
    for ( int j = 0, len = ExpGetPmdMorphNum(i); j < len; ++j )
    {
      auto name = ExpGetPmdMorphName(i, j);
      for ( int k = 0; k < morph_resource_size; ++k )
      {
        if ( strcmp(getName(static_cast<MorphKind>(k)), name) == 0 )
        {
          morph_id_[k] = j;
          break;
        }
      }
    }
    bone_id_.fill(-1);
    for ( int j = 0, len = ExpGetPmdBoneNum(i); j < len; ++j )
    {
      auto name = ExpGetPmdBoneName(i, j);
      for ( int k = 0; k < bone_resource_size; ++k )
      {
        if ( strcmp(getName(static_cast<BoneKind>(k)), name) == 0 )
        {
          bone_id_[k] = j;
          break;
        }
      }
    }
  }

  float PMDResource::triggerVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::trigger_morph)); }

  float PMDResource::autoPlayVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::auto_play_morph)); }

  float PMDResource::frameVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::frame_morph)); }

  float PMDResource::loopVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::loop_morph)); }

  float PMDResource::triggerEraseVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::trigger_erase_morph)); }

  float PMDResource::scaleUpVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::scale_up_morph)); }

  float PMDResource::scaleDown(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::scale_down_morph)); }

  float PMDResource::speedUpVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::speed_up_morph)); }

  float PMDResource::speedDownVal(int i) const { return ExpGetPmdMorphValue(i, getID(MorphKind::speed_down_morph)); }


  D3DMATRIX PMDResource::playBone(int i) const { return getBone(i, BoneKind::play_bone); }

  D3DMATRIX PMDResource::centerBone(int i) const { return getBone(i, BoneKind::center_bone); }

  D3DMATRIX PMDResource::baseBone(int i) const { return getBone(i, BoneKind::base_bone); }

  MyEffect::MyEffect() : now_frame_(0), manager_(nullptr), handle_(-1), effect_(nullptr), resource(-1) {}

  MyEffect::MyEffect(Effekseer::Manager* manager, Effekseer::Effect* effect, PMDResource resource) : manager_(manager), effect_(effect), resource(resource)
  {
    create();
  }

  MyEffect::~MyEffect() {}

  void MyEffect::setMatrix(const D3DMATRIX& center, const D3DMATRIX& base)
  {
    Effekseer::Matrix44 tmp;
    Effekseer::Matrix44::Inverse(tmp, toMatrix4x4(base));
    base_matrix_ = toMatrix4x3(base);
    Effekseer::Matrix44::Mul(tmp, toMatrix4x4(center), tmp);
    for ( int i = 0; i < 4; ++i )
    {
      for ( int j = 0; j < 3; ++j )
      {
        matrix_.Value[i][j] = tmp.Values[i][j];
      }
    }
  }

  void MyEffect::setScale(float x, float y, float z)
  {
    scale_.X = x;
    scale_.Y = y;
    scale_.Z = z;
  }

  void MyEffect::frameTypeUpdate(float new_frame)
  {
    if ( now_frame_ > new_frame + eps )
    {
      manager_->StopEffect(handle_);
      handle_ = -1;
      now_frame_ = new_frame;
      return;
    }
    else ifCreate();
#if 1
    const int len = std::min(static_cast<int>(1e9), static_cast<int>(new_frame - now_frame_));
    for ( int i = 0; i < len; i++ )
    {
      UpdateMainHandle(1.0f);
    }
    //manager_->UpdateHandle(handle_, new_frame - now_frame_ - len);
    if ( len == 0 )
    {
      UpdateMainHandle(0.0f);
    }
#else
    manager_->BeginUpdate();
    manager_->UpdateHandle(handle_, new_frame - now_frame_);
    manager_->EndUpdate();
    now_frame_ = new_frame;
#endif
  }

  void MyEffect::autoPlayTypeUpdate(int i)
  {
    if ( resource.loopVal(i) > 1.0f - eps ) ifCreate();
    const int delta_frame = deltaFrame();
    if ( delta_frame < 0 )
    {
      if ( manager_->Exists(handle_) ) manager_->StopEffect(handle_);
      now_frame_ = 0.0f;
      handle_ = -1;
    }
    if ( handle_ == -1 ) return;
    for ( int j = 0; j < delta_frame; ++j )
    {
      UpdateMainHandle(getSpeed(i));
    }
  }

  void MyEffect::draw() const
  {
    if ( handle_ != -1 ) manager_->DrawHandle(handle_);

    for ( auto& i : trigger_type_effect_ )
    {
      manager_->DrawHandle(i);
    }
  }

  void MyEffect::pushTriggerType()
  {
    auto h = manager_->Play(effect_, 0.0f, 0.0f, 0.0f);
    trigger_type_effect_.push_back(h);
    manager_->Flip();
    UpdateHandle(h, 0.0f);
  }

  void MyEffect::triggerTypeUpdate(int i)
  {
    auto is_trigger = resource.triggerVal(i) >= 1.0f - eps;
    if ( is_trigger )
    {
      if ( pre_triggerd_ == false )
      {
        pushTriggerType();
        pre_triggerd_ = true;
      }
    }
    else
    {
      pre_triggerd_ = false;
    }

    const int delta_frame = deltaFrame();
    if ( delta_frame < 0 || resource.triggerEraseVal(i) >= 1.0f - eps )
    {
      trigger_type_effect_.clear();
    }

    // 再生が終了したものを削除
    auto e = std::remove_if(trigger_type_effect_.begin(), trigger_type_effect_.end(), [this](Effekseer::Handle h)
                            {
                              return !manager_->Exists(h);
                            });
    trigger_type_effect_.resize(distance(trigger_type_effect_.begin(), e));

    manager_->BeginUpdate();
    for ( auto& j : trigger_type_effect_ )
    {
      for ( int k = 0; k < delta_frame; ++k )
      {
        manager_->UpdateHandle(j, getSpeed(i));
      }
    }
    UpdateMainHandle(0.0f);
    manager_->EndUpdate();
  }

  void MyEffect::OnLostDevice() const
  {
    if ( effect_ ) effect_->UnloadResources();
  }

  void MyEffect::OnResetDevice() const
  {
    if ( effect_ ) effect_->ReloadResources();
  }

  float MyEffect::getSpeed(int i) const { return 1.0f + resource.speedUpVal(i) - resource.speedDownVal(i); }

  void MyEffect::update(int i)
  {
    // 座標の処理
    auto center = resource.centerBone(i);
    auto base_center = resource.baseBone(i);
    setMatrix(center, base_center);


    // 拡縮処理
    auto scale = resource.scaleUpVal(i) * 10 - resource.scaleDown(i) + 1.0f;
    setScale(scale, scale, scale);

    float auto_play_val = resource.autoPlayVal(i);
    if ( auto_play_val >= 1.0f - eps )
    {
      // オート再生方式
      autoPlayTypeUpdate(i);
    }
    else
    {
      // フレーム方式
      auto play_mat = resource.playBone(i);
      double play_time = play_mat.m[3][1] + resource.frameVal(i) * 100.0f;
      play_time = play_time - 0.5;

      frameTypeUpdate(static_cast<float>(play_time));
    }

    // トリガー方式
    triggerTypeUpdate(i);

    pre_mmd_time_ = ExpGetFrameTime();
  }

  int MyEffect::deltaFrame() const
  {
    float time = ExpGetFrameTime();
    return (time - pre_mmd_time_) * 60;
  }

  void MyEffect::ifCreate()
  {
    if ( !manager_->Exists(handle_) ) create();
  }

  void MyEffect::create()
  {
    if ( manager_->Exists(handle_) ) manager_->StopEffect(handle_);
    handle_ = manager_->Play(effect_, 0.0f, 0.0f, 0.0f);
    manager_->Flip();
    now_frame_ = 0.0;
    printf("create %f\n", ExpGetFrameTime());
  }

  void MyEffect::UpdateMainHandle(float delta_time)
  {
    UpdateHandle(handle_, delta_time);
    now_frame_ += delta_time;
  }

  void MyEffect::UpdateHandle(Effekseer::Handle h, float delta_time)
  {
    manager_->BeginUpdate();
    manager_->SetMatrix(h, matrix_);
    manager_->SetBaseMatrix(h, base_matrix_);
    manager_->SetScale(h, scale_.X, scale_.Y, scale_.Z);
    manager_->UpdateHandle(h, delta_time);
    manager_->EndUpdate();
  }

  DistortingCallback::DistortingCallback(::EffekseerRendererDX9::Renderer* renderer, LPDIRECT3DDEVICE9 device,
                                         int texWidth, int texHeight) : renderer(renderer), device(device), width_(texWidth), height_(texHeight)
  {
    OnResetDevice();
  }

  DistortingCallback::~DistortingCallback()
  {
    ES_SAFE_RELEASE(texture);
  }

  void DistortingCallback::OnDistorting()
  {
    if ( use_distoring_ == false ) return;

    Microsoft::WRL::ComPtr<IDirect3DSurface9> texSurface;

    HRESULT hr = texture->GetSurfaceLevel(0, texSurface.ReleaseAndGetAddressOf());
    if ( FAILED(hr) )
    {
      std::wstring error = __FUNCTIONW__"でエラーが発生しました。\ntexture->GetSurfaceLevel : " + std::to_wstring(hr);
      MessageBoxW(nullptr, error.c_str(), L"エラー", MB_OK);
      use_distoring_ = false;
      return;
    }

    Microsoft::WRL::ComPtr<IDirect3DSurface9> targetSurface;
    hr = device->GetRenderTarget(0, targetSurface.ReleaseAndGetAddressOf());
    if ( FAILED(hr) )
    {
      std::wstring error = __FUNCTIONW__"でエラーが発生しました。\ntexture->GetSurfaceLevel : " + std::to_wstring(hr);
      MessageBoxW(nullptr, error.c_str(), L"エラー", MB_OK);
      use_distoring_ = false;
      return;
    }

    D3DVIEWPORT9 viewport;
    device->GetViewport(&viewport);
    RECT rect{
      static_cast<LONG>(viewport.X),
      static_cast<LONG>(viewport.Y),
      static_cast<LONG>(viewport.Width + viewport.X),
      static_cast<LONG>(viewport.Height + viewport.Y)
    };
    hr = device->StretchRect(targetSurface.Get(), &rect, texSurface.Get(), nullptr, D3DTEXF_NONE);
    if ( FAILED(hr) )
    {
      std::wstring error = __FUNCTIONW__"でエラーが発生しました。\ntexture->GetSurfaceLevel : " + std::to_wstring(hr);
      MessageBoxW(nullptr, error.c_str(), L"エラー", MB_OK);
      use_distoring_ = false;
      return;
    }

    renderer->SetBackground(texture);
  }

  void DistortingCallback::OnLostDevice()
  {
    ES_SAFE_RELEASE(texture);
  }

  void DistortingCallback::OnResetDevice()
  {
    OnLostDevice();
    device->CreateTexture(width_, height_, 1, D3DUSAGE_RENDERTARGET,
                          D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);
  }

  D3D9DeviceEffekserr::D3D9DeviceEffekserr(IDirect3DDevice9* device) : now_present_(false), device_(device)
  {
    device_->AddRef();
    HookAPI();
    renderer_ = ::EffekseerRendererDX9::Renderer::Create(device, 10000);

    // エフェクト管理用インスタンスの生成
    manager_ = ::Effekseer::Manager::Create(10000);

    // 描画用インスタンスから描画機能を設定
    manager_->SetSpriteRenderer(renderer_->CreateSpriteRenderer());
    manager_->SetRibbonRenderer(renderer_->CreateRibbonRenderer());
    manager_->SetRingRenderer(renderer_->CreateRingRenderer());
    manager_->SetTrackRenderer(renderer_->CreateTrackRenderer());
    manager_->SetModelRenderer(renderer_->CreateModelRenderer());

    // 描画用インスタンスからテクスチャの読込機能を設定
    // 独自拡張可能、現在はファイルから読み込んでいる。
    manager_->SetTextureLoader(renderer_->CreateTextureLoader());
    manager_->SetModelLoader(renderer_->CreateModelLoader());

    // 投影行列を設定
    renderer_->SetProjectionMatrix(
      ::Effekseer::Matrix44().PerspectiveFovRH(90.0f / 180.0f * 3.14f, 1024.0f / 768.0f, 1.0f, 500000.0f));

    SetDistorting();
  }

  D3D9DeviceEffekserr::~D3D9DeviceEffekserr()
  {
    manager_->Destroy();
    renderer_->Destory();
    RestoreHook();
    device_->Release();
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
  }

  void D3D9DeviceEffekserr::DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
  {
    const int pmd_num = ExpGetPmdNum();
    const int technic_type = ExpGetCurrentTechnic();
    const int now_render_object = ExpGetCurrentObject();
    const int now_render_material = ExpGetCurrentMaterial();
    UpdateProjection();
    if ( D3DPT_LINELIST != Type && now_render_material == 0 && now_render_object != 0
      && !now_present_ && (technic_type == 1 || technic_type == 2) )
      for ( int i = 0; i < pmd_num; i++ )
      {
        if ( now_render_object != ExpGetPmdOrder(i) ) continue;

        UpdateCamera();
        const int ID = ExpGetPmdID(i);
        auto it = effect_.find(ID);
        if ( it != effect_.end() )
        {
          auto& effect = it->second;

          effect.update(i);

          now_present_ = true;

          // Effekseerライブラリがリストアしてくれないので自前でバックアップしてる
          float constant_data[256 * 4];
          device_->GetVertexShaderConstantF(0, constant_data, sizeof(constant_data) / sizeof(float) / 4);

          UINT stride;
          IDirect3DVertexBuffer9* stream_data;
          UINT offset;
          device_->GetStreamSource(0, &stream_data, &offset, &stride);

          IDirect3DIndexBuffer9* index_data;
          device_->GetIndices(&index_data);

          // エフェクトの描画開始処理を行う。
          if ( renderer_->BeginRendering() )
          {
            // エフェクトの描画を行う。
            effect.draw();

            // エフェクトの描画終了処理を行う。
            renderer_->EndRendering();
          }

          device_->SetStreamSource(0, stream_data, offset, stride);
          if ( stream_data ) stream_data->Release();

          device_->SetIndices(index_data);
          if ( index_data ) index_data->Release();

          device_->SetVertexShaderConstantF(0, constant_data, sizeof(constant_data) / sizeof(float) / 4);

          now_present_ = false;
        }
      }
  }

  void D3D9DeviceEffekserr::BeginScene(void)
  {
    int len = ExpGetPmdNum();
    if ( len != effect_.size() )
      for ( int i = 0; i < len; i++ )
      {
        const int id = ExpGetPmdID(i);
        const auto file_name = ExpGetPmdFilename(i);
        filesystem::path path(file_name);
        if ( ".efk" == path.extension() )
        {
          auto it = effect_.insert({ id, MyEffect() });
          if ( !it.second ) continue;

          // エフェクトの読込
          nowEFKLoading = true;
          auto eff = Effekseer::Effect::Create(manager_, reinterpret_cast<const EFK_CHAR*>((path.remove_filename() / path.stem().stem()).c_str()));
          nowEFKLoading = false;
          it.first->second = MyEffect(manager_, eff, PMDResource(i));
        }
      }
  }

  void D3D9DeviceEffekserr::EndScene(void)
  {
    printf("%f\n", ExpGetFrameTime());
  }


  void D3D9DeviceEffekserr::UpdateCamera() const
  {
    D3DMATRIX view, world;
    device_->GetTransform(D3DTS_WORLD, &world);
    device_->GetTransform(D3DTS_VIEW, &view);
    Effekseer::Matrix44 camera, eview = toMatrix4x4(view), eworld = toMatrix4x4(world);
    Effekseer::Matrix44::Mul(camera, eworld, eview);

    renderer_->SetCameraMatrix(camera);
  }

  void D3D9DeviceEffekserr::UpdateProjection() const
  {
    D3DMATRIX projection;
    device_->GetTransform(D3DTS_PROJECTION, &projection);
    Effekseer::Matrix44 mat = toMatrix4x4(projection);
    renderer_->SetProjectionMatrix(mat);
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

  void D3D9DeviceEffekserr::RestoreHook()
  {
    hook_rewrite::RestoreCreateFileW();
    hook_rewrite::RestoreCloseHandle();
    hook_rewrite::RestoreReadFile();
    hook_rewrite::RestoreSetFilePointer();
    //hook_rewrite::RewriteDragFinish();
    HMODULE handle = LoadLibrary("shell32.dll");
    PFDragQueryFileW = reinterpret_cast<FDragQueryFileW>(GetProcAddress(handle, "DragQueryFileW"));
    modifyIAT("shell32.dll", myDragQueryFileW, PFDragQueryFileW);

    PFDragFinish = reinterpret_cast<FDragFinish>(GetProcAddress(handle, "DragFinish"));
    modifyIAT("shell32.dll", myDragFinish, PFDragFinish);
  }

  void D3D9DeviceEffekserr::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
  {
    renderer_->OnLostDevice();
    for ( auto& i : effect_ )
    {
      i.second.OnLostDevice();
    }
    distorting_callback_->OnLostDevice();
  }

  void D3D9DeviceEffekserr::PostReset(D3DPRESENT_PARAMETERS* pPresentationParameters, HRESULT& res)
  {
    distorting_callback_->OnResetDevice();
    for ( auto& i : effect_ )
    {
      i.second.OnResetDevice();
    }
    renderer_->OnResetDevice();
  }

  void D3D9DeviceEffekserr::SetDistorting()
  {
    // テクスチャサイズの取得
    Microsoft::WRL::ComPtr<IDirect3DSurface9> tex;
    if ( FAILED(device_->GetRenderTarget(0, tex.ReleaseAndGetAddressOf())) )
    {
      MessageBoxW(nullptr, L"レンダーターゲットの取得に失敗しました。\nディストーション（歪み）は使用できません。", L"エラー", MB_OK);
      return;
    }
    D3DSURFACE_DESC desc;
    if ( SUCCEEDED(tex->GetDesc(&desc)) )
    {
      distorting_callback_ = new DistortingCallback(renderer_, device_, desc.Width, desc.Height);
      renderer_->SetDistortingCallback(distorting_callback_);
    }
    else
    {
      MessageBoxW(nullptr, L"レンダーターゲットの画面サイズの取得に失敗しました。\nディストーション（歪み）は使用できません。", L"エラー", MB_OK);
    }
  }
}

int version() { return 2; }

MMDPluginDLL2* create2(IDirect3DDevice9* device) { return new efk::D3D9DeviceEffekserr(device); }

void destroy2(MMDPluginDLL2* p) { delete p; }
