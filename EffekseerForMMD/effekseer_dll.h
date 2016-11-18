#pragma once
#include <unordered_map>
#include "MMDPlugin/mmd_plugin.h"
#include <EffekseerRendererDX9.h>
#include "mmd/MMDExport.h"

namespace efk
{
  struct PMDResource
  {
    PMDResource(int i);

    static const char* getTriggerMorphName();

    float triggerVal(int i) const;

  private:
    int trigger_morph_id_ = -1;
  };

  struct MyEffect
  {
    MyEffect();
    MyEffect(Effekseer::Manager* manager, Effekseer::Effect* effect, PMDResource resource);

    ~MyEffect();
    void setMatrix(const D3DMATRIX& center, const D3DMATRIX& base);
    void setBaseMatrix(const D3DMATRIX& mat);
    void setMatrix(const D3DMATRIX& mat);

    void update(float delta_frame);

    void draw() const;

    void pushTriggerType();
    void triggerTypeUpdate();

    bool pre_triggerd_ = false;

    PMDResource resource;

  private:

    void ifCreate();

    void create();

    Effekseer::Matrix43 base_matrix;
    float now_frame;
    Effekseer::Manager* manager;
    Effekseer::Handle handle;
    Effekseer::Effect* effect;
    Effekseer::Matrix43 matrix;
    std::vector<Effekseer::Handle> trigger_type_effect_;
  };

  struct D3D9DeviceEffekserr : public MMDPluginDLL2
  {
    D3D9DeviceEffekserr(IDirect3DDevice9* device);

    void UpdateCamera() const;
    void UpdateProjection() const;

    void HookAPI();

    void DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
    void BeginScene(THIS) override;
    void EndScene(THIS) override;

    void Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;
    void PostReset(D3DPRESENT_PARAMETERS* pPresentationParameters, HRESULT& res) override;

  private:

    EffekseerRendererDX9::Renderer* renderer_;
    Effekseer::Manager* manager_;

    bool now_present_;
    std::unordered_map<std::wstring, int> effect_id_;
    // <ID, Effect>
    std::unordered_map<int, MyEffect> effect_;
    IDirect3DDevice9* device_;
  };
}

extern "C"
{
  MMD_PLUGIN_API int version();
  MMD_PLUGIN_API MMDPluginDLL1* create1(IDirect3DDevice9* device);
  MMD_PLUGIN_API void destroy1(MMDPluginDLL1* p);
}
