#pragma once
#include <unordered_map>
#include "MMDPlugin/mmd_plugin.h"
#include <EffekseerRendererDX9.h>

namespace efk
{
  struct MyEffect
  {
    MyEffect();

    MyEffect(Effekseer::Manager* manager, Effekseer::Effect* effect);

    ~MyEffect();
    void setMatrix(const D3DMATRIX& mat) const;

    void update(float delta_frame);

    void draw() const;

  private:

    void ifCreate();

    void create();

    float now_frame;
    Effekseer::Manager* manager;
    Effekseer::Handle handle;
    Effekseer::Effect* effect;
  };

  struct D3D9DeviceEffekserr : MMDPluginDLL1
  {
    D3D9DeviceEffekserr(IDirect3DDevice9* device);

    virtual void DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
    virtual void BeginScene(THIS) override;
    virtual void EndScene(THIS) override;

    void UpdateCamera() const;
    void UpdateProjection() const;
    EffekseerRendererDX9::Renderer* g_renderer;
    Effekseer::Manager* g_manager;
    //Effekseer::Handle g_handle;

    bool now_present;
    std::unordered_map<std::wstring, int> effectID;
    // <ID, Effect>
    std::unordered_map<int, MyEffect> effect;
  private:
    IDirect3DDevice9* device;
  };
}

extern "C"
{
  MMD_PLUGIN_API int version();
  MMD_PLUGIN_API MMDPluginDLL1* create1(IDirect3DDevice9* device);
  MMD_PLUGIN_API void destroy1(MMDPluginDLL1* p);
}
