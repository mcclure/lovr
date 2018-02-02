#include "api.h"
#include "graphics/graphics.h"
#include "graphics/canvas.h"

int l_lovrCanvasRenderTo(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int nargs = lua_gettop(L) - 2;
  lovrGraphicsPushView();
  lovrCanvasBind(canvas);
  lua_call(L, nargs, 0);
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

int l_lovrCanvasExportFile(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  Texture* texture = &canvas->texture;
  const char *str = luaL_checkstring(L, 2);
  if (!lovrTextureExportFile(texture, str))
    luaL_error(L, "Failed to write PNG file");
  return 0;
}

const luaL_Reg lovrCanvas[] = {
  { "renderTo", l_lovrCanvasRenderTo },
  { "getSampleFilter", l_lovrCanvasGetSampleFilter },
  { "setSampleFilter", l_lovrCanvasSetSampleFilter },
  { "getFormat", l_lovrCanvasGetFormat },
  { "getMSAA", l_lovrCanvasGetMSAA },
  { "exportFile", l_lovrCanvasExportFile },
  { NULL, NULL }
};
