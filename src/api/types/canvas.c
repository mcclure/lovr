#include "api.h"
#include "graphics/graphics.h"
#include "graphics/canvas.h"

int l_lovrCanvasRenderTo(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  lovrGraphicsPushView();
  lovrCanvasBind(canvas);
  lua_settop(L, 2);
  lua_call(L, 0, 0);
  lovrCanvasResolveMSAA(canvas);
  lovrGraphicsPopView();
  return 0;
}

int l_lovrCanvasGetFormat(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  TextureFormat format = lovrCanvasGetFormat(canvas);
  luax_pushenum(L, &TextureFormats, format);
  return 1;
}

int l_lovrCanvasGetSampleFilter(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  SampleFilter sampleFilter = lovrCanvasGetSampleFilter(canvas);
  luax_pushenum(L, &SampleFilters, sampleFilter);
  return 1;
}

int l_lovrCanvasSetSampleFilter(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  SampleFilter sampleFilter = *(SampleFilter*) luax_optenum(L, 2, "weightedaverage", &SampleFilters, "sample filter");
  lovrCanvasSetSampleFilter(canvas, sampleFilter);
  return 0;
}

int l_lovrCanvasGetMSAA(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  lua_pushinteger(L, lovrCanvasGetMSAA(canvas));
  return 1;
}

const luaL_Reg lovrCanvas[] = {
  { "renderTo", l_lovrCanvasRenderTo },
  { "getSampleFilter", l_lovrCanvasGetSampleFilter },
  { "setSampleFilter", l_lovrCanvasSetSampleFilter },
  { "getFormat", l_lovrCanvasGetFormat },
  { "getMSAA", l_lovrCanvasGetMSAA },
  { NULL, NULL }
};
