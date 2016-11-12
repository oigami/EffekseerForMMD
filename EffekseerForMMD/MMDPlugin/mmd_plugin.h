#pragma once
#include <d3d9.h>
#define MMD_PLUGIN_API __declspec(dllexport)

class MMDPluginDLL1
{
public:

  virtual ~MMDPluginDLL1() {}

  // callback
  // すべて実際の関数が呼ばれる前にコールバックされる
  /*
  // 内部実装
  MMDPluginDLL1::QueryInterface(riid, ppvObj);
  return d3dDevice->QueryInterface(riid, ppvObj);
  */
  virtual void QueryInterface(REFIID riid, void** ppvObj) {}
  virtual void AddRef() {}
  virtual void Release() {}
  virtual void TestCooperativeLevel() {}
  virtual void GetAvailableTextureMem() {}
  virtual void EvictManagedResources() {}
  virtual void GetDirect3D(IDirect3D9** ppD3D9) {}
  virtual void GetDeviceCaps(D3DCAPS9* pCaps) {}
  virtual void GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) {}
  virtual void GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) {}
  virtual void SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {}
  virtual void SetCursorPosition(int X, int Y, DWORD Flags) {}
  virtual void ShowCursor(BOOL bShow) {}
  virtual void CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) {}
  virtual void GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) {}
  virtual void GetNumberOfSwapChains() {}
  virtual void Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) {}
  virtual void Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion) {}
  virtual void GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) {}
  virtual void GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {}
  virtual void SetDialogBoxMode(BOOL bEnableDialogs) {}
  virtual void SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {}
  virtual void GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) {}
  virtual void CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {}
  virtual void CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {}
  virtual void CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) {}
  virtual void CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {}
  virtual void CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) {}
  virtual void CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {}
  virtual void CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {}
  virtual void UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) {}
  virtual void UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {}
  virtual void GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) {}
  virtual void GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) {}
  virtual void StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {}
  virtual void ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) {}
  virtual void CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {}
  virtual void SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {}
  virtual void GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) {}
  virtual void SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) {}
  virtual void GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) {}
  virtual void BeginScene() {}
  virtual void EndScene() {}
  virtual void Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {}
  virtual void SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {}
  virtual void GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {}
  virtual void MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {}
  virtual void SetViewport(CONST D3DVIEWPORT9* pViewport) {}
  virtual void GetViewport(D3DVIEWPORT9* pViewport) {}
  virtual void SetMaterial(CONST D3DMATERIAL9* pMaterial) {}
  virtual void GetMaterial(D3DMATERIAL9* pMaterial) {}
  virtual void SetLight(DWORD Index, CONST D3DLIGHT9* pLight) {}
  virtual void GetLight(DWORD Index, D3DLIGHT9* pLight) {}
  virtual void LightEnable(DWORD Index, BOOL Enable) {}
  virtual void GetLightEnable(DWORD Index, BOOL* pEnable) {}
  virtual void SetClipPlane(DWORD Index, CONST float* pPlane) {}
  virtual void GetClipPlane(DWORD Index, float* pPlane) {}
  virtual void SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {}
  virtual void GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {}
  virtual void CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) {}
  virtual void BeginStateBlock() {}
  virtual void EndStateBlock(IDirect3DStateBlock9** ppSB) {}
  virtual void SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus) {}
  virtual void GetClipStatus(D3DCLIPSTATUS9* pClipStatus) {}
  virtual void GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) {}
  virtual void SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) {}
  virtual void GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {}
  virtual void SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {}
  virtual void GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) {}
  virtual void SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {}
  virtual void ValidateDevice(DWORD* pNumPasses) {}
  virtual void SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {}
  virtual void GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) {}
  virtual void SetCurrentTexturePalette(UINT PaletteNumber) {}
  virtual void GetCurrentTexturePalette(UINT* PaletteNumber) {}
  virtual void SetScissorRect(CONST RECT* pRect) {}
  virtual void GetScissorRect(RECT* pRect) {}
  virtual void SetSoftwareVertexProcessing(BOOL bSoftware) {}
  virtual void GetSoftwareVertexProcessing() {}
  virtual void SetNPatchMode(float nSegments) {}
  virtual void GetNPatchMode() {}
  virtual void DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {}
  virtual void DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {}
  virtual void DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {}
  virtual void DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {}
  virtual void ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {}
  virtual void CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {}
  virtual void SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) {}
  virtual void GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) {}
  virtual void SetFVF(DWORD FVF) {}
  virtual void GetFVF(DWORD* pFVF) {}
  virtual void CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) {}
  virtual void SetVertexShader(IDirect3DVertexShader9* pShader) {}
  virtual void GetVertexShader(IDirect3DVertexShader9** ppShader) {}
  virtual void SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {}
  virtual void GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) {}
  virtual void SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {}
  virtual void GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) {}
  virtual void SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) {}
  virtual void GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {}
  virtual void SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {}
  virtual void GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) {}
  virtual void SetStreamSourceFreq(UINT StreamNumber, UINT Setting) {}
  virtual void GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) {}
  virtual void SetIndices(IDirect3DIndexBuffer9* pIndexData) {}
  virtual void GetIndices(IDirect3DIndexBuffer9** ppIndexData) {}
  virtual void CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {}
  virtual void SetPixelShader(IDirect3DPixelShader9* pShader) {}
  virtual void GetPixelShader(IDirect3DPixelShader9** ppShader) {}
  virtual void SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {}
  virtual void GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) {}
  virtual void SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {}
  virtual void GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) {}
  virtual void SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) {}
  virtual void GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {}
  virtual void DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {}
  virtual void DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {}
  virtual void DeletePatch(UINT Handle) {}
  virtual void CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) {}
};

extern "C"
{
  MMD_PLUGIN_API int version();
  MMD_PLUGIN_API MMDPluginDLL1* create1(IDirect3DDevice9* device);
  MMD_PLUGIN_API void destroy1(MMDPluginDLL1* p);
}
