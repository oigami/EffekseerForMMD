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


    float triggerVal(int i) const;
    float autoPlayVal(int i) const;
    float frameVal(int i) const;

    D3DMATRIX playBone(int i) const;
    D3DMATRIX centerBone(int i) const;
    D3DMATRIX baseBone(int i) const;

  private:

    static const char* getTriggerMorphName();
    static const char* getAutoPlayMorphName();
    static const char* getFrameMorphName();

    static const char* getPlayBoneName();
    static const char* getCenterBone();
    static const char* getBaseBone();

    int trigger_morph_id_ = -1;
    int auto_play_morph_id_ = -1;
    int frame_morph_id_ = -1;

    int play_bone_id_ = -1;
    int center_bone_id_ = -1;
    int base_bone_id_ = -1;
  };

  struct MyEffect
  {
    MyEffect();
    MyEffect(Effekseer::Manager* manager, Effekseer::Effect* effect, PMDResource resource);

    ~MyEffect();
    void setMatrix(const D3DMATRIX& center, const D3DMATRIX& base, const D3DMATRIX& view);

    void update(float delta_frame);

    void AutoPlayTypeUpdate();

    void draw() const;

    void pushTriggerType();
    void triggerTypeUpdate(int i);

    void OnLostDevice() const;

    void OnResetDevice() const;


    PMDResource resource;

  private:

    void ifCreate();

    void create();

    void UpdateHandle(float time);

    bool pre_triggerd_ = false;
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

    ~D3D9DeviceEffekserr()
    {
      RestoreHook();
    }

    void UpdateCamera() const;
    void UpdateProjection() const;

    void HookAPI();
    void RestoreHook();

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
