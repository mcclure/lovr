#include "api.h"
#include "filesystem/filesystem.h"
#include "data/blob.h"
#include <stdlib.h>

bool lovrFilesystemReloadEnable;

struct LovrLiveReloadRecord;
typedef struct LovrLiveReloadRecord {
  char *path;
  long modtime;
  struct LovrLiveReloadRecord *next;
} LovrLiveReloadRecord;
static LovrLiveReloadRecord *liveRecordRoot = NULL;

static void lovrReloadAddWatch(const char *physPath) {
  if (!lovrFilesystemReloadEnable)
    return;
  if (PHYSFS_getRealDir(physPath)) { // Disregard anything in a zip file, etc
    LovrLiveReloadRecord *newRecord = malloc(sizeof(LovrLiveReloadRecord));
    newRecord->path = strdup(physPath);
    newRecord->modtime = lovrFilesystemGetLastModified(physPath);
    newRecord->next = liveRecordRoot;
    liveRecordRoot = newRecord;
  }
}

static bool lovrReloadCheck() {
  for(LovrLiveReloadRecord *i = liveRecordRoot; i; i = i->next) {
    if (lovrFilesystemGetLastModified(i->path) != i->modtime)
      return true;
  }
  return false;
}

// Returns a Blob, leaving stack unchanged.  The Blob must be released when finished.
Blob* luax_readblob(lua_State* L, int index, const char* debug) {
  if (lua_type(L, index) == LUA_TUSERDATA) {
    Blob* blob = luax_checktype(L, index, Blob);
    lovrRetain(blob);
    return blob;
  } else {
    const char* path = luaL_checkstring(L, index);

    size_t size;
    void* data = lovrFilesystemRead(path, &size);
    if (!data) {
      luaL_error(L, "Could not read %s from '%s'", debug, path);
    }

    return lovrBlobCreate(data, size, path);
  }
}

static int pushDirectoryItem(void* userdata, const char* path, const char* filename) {
  lua_State* L = userdata;
  int n = lua_objlen(L, -1);
  lua_pushstring(L, filename);
  lua_rawseti(L, -2, n + 1);
  return 1;
}

int l_lovrFilesystemLoad(lua_State* L);

static int moduleLoader(lua_State* L) {
  const char* module = luaL_gsub(L, lua_tostring(L, -1), ".", "/");
  lua_pop(L, 2);

  char* path; int i;
  vec_foreach(lovrFilesystemGetRequirePath(), path, i) {
    const char* filename = luaL_gsub(L, path, "?", module);
    if (lovrFilesystemIsFile(filename)) {
      if (lovrFilesystemReloadEnable) {
        lovrReloadAddWatch(filename);
      }
      return l_lovrFilesystemLoad(L);
    }
    lua_pop(L, 1);
  }

  return 0;
}

static const char* libraryExtensions[] = {
#ifdef _WIN32
  ".dll", NULL
#elif __APPLE__
  ".so", ".dylib", NULL
#else
  ".so", NULL
#endif
};

static int libraryLoader(lua_State* L) {
  const char* modulePath = luaL_gsub(L, lua_tostring(L, -1), ".", "/");
  const char* moduleFunction = luaL_gsub(L, lua_tostring(L, -1), ".", "_");
  char* hyphen = strchr(moduleFunction, '-');
  moduleFunction = hyphen ? hyphen + 1 : moduleFunction;

  lua_pop(L, 3);

  char* path; int i;
  vec_foreach(lovrFilesystemGetCRequirePath(), path, i) {
    for (const char** extension = libraryExtensions; *extension != NULL; extension++) {
      char buffer[64];
      snprintf(buffer, 63, "%s%s", modulePath, *extension);
      const char* filename = luaL_gsub(L, path, "??", buffer);
      filename = luaL_gsub(L, filename, "?", modulePath);
      lua_pop(L, 2);

      if (lovrFilesystemIsFile(filename)) {
        const char* realPath = lovrFilesystemGetRealDirectory(filename);
        char fullPath[LOVR_PATH_MAX];
        snprintf(fullPath, LOVR_PATH_MAX, "%s%c%s", realPath, lovrDirSep, filename);
        void* library = lovrLoadLibrary(fullPath);

        snprintf(buffer, 63, "luaopen_%s", moduleFunction);
        void* function = lovrLoadSymbol(library, buffer);
        if (!function) {
          snprintf(buffer, 63, "loveopen_%s", moduleFunction);
          function = lovrLoadSymbol(library, buffer);
          if (!function) {
            snprintf(buffer, 63, "lovropen_%s", moduleFunction);
            function = lovrLoadSymbol(library, buffer);
          }
        }

        if (function) {
          lua_pushcfunction(L, (lua_CFunction) function);
          return 1;
        } else {
          lovrCloseLibrary(library);
        }

        if (lovrFilesystemReloadEnable) {
          lovrReloadAddWatch(filename);
        }
      }
    }
  }

  return 0;
}

int l_lovrFilesystemInit(lua_State* L) {
  if (liveRecordRoot) {
    LovrLiveReloadRecord *i = liveRecordRoot;
    while (i) {
      LovrLiveReloadRecord *current = i;
      i = current->next;
      free(current->path);
      free(current);
    }
    liveRecordRoot = NULL;
  }
    
  lua_newtable(L);
  luaL_register(L, NULL, lovrFilesystem);

  lua_getglobal(L, "arg");
  lua_rawgeti(L, -1, -2);
  lua_rawgeti(L, -2, 1);
  const char* arg0 = lua_tostring(L, -2);
  const char* arg1 = lua_tostring(L, -1);
  lovrFilesystemInit(arg0, arg1);
  lua_pop(L, 3);

  luax_registerloader(L, moduleLoader, 2);
  luax_registerloader(L, libraryLoader, 3);
  return 1;
}

int l_lovrFilesystemAppend(lua_State* L) {
  size_t size;
  const char* path = luaL_checkstring(L, 1);
  const char* content = luaL_checklstring(L, 2, &size);
  lua_pushnumber(L, lovrFilesystemWrite(path, content, size, 1));
  return 1;
}

int l_lovrFilesystemCreateDirectory(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, !lovrFilesystemCreateDirectory(path));
  return 1;
}

int l_lovrFilesystemGetAppdataDirectory(lua_State* L) {
  char buffer[LOVR_PATH_MAX];

  if (lovrFilesystemGetAppdataDirectory(buffer, sizeof(buffer))) {
    lua_pushnil(L);
  } else {
    lua_pushstring(L, buffer);
  }

  return 1;
}

int l_lovrFilesystemGetDirectoryItems(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_newtable(L);
  lovrFilesystemGetDirectoryItems(path, pushDirectoryItem, L);
  return 1;
}

int l_lovrFilesystemGetExecutablePath(lua_State* L) {
  char buffer[LOVR_PATH_MAX];

  if (lovrFilesystemGetExecutablePath(buffer, sizeof(buffer))) {
    lua_pushnil(L);
  } else {
    lua_pushstring(L, buffer);
  }

  return 1;
}

int l_lovrFilesystemGetIdentity(lua_State* L) {
  const char* identity = lovrFilesystemGetIdentity();
  if (identity) {
    lua_pushstring(L, identity);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int l_lovrFilesystemGetLastModified(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  int lastModified = lovrFilesystemGetLastModified(path);

  if (lastModified < 0) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, lastModified);
  }

  return 1;
}

int l_lovrFilesystemGetRealDirectory(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushstring(L, lovrFilesystemGetRealDirectory(path));
  return 1;
}

static void pushRequirePath(lua_State* L, vec_str_t* path) {
  char* pattern; int i;
  vec_foreach(path, pattern, i) {
    lua_pushstring(L, pattern);
    lua_pushstring(L, ";");
  }
  lua_pop(L, 1);
  lua_concat(L, path->length * 2 - 1);
}

int l_lovrFilesystemGetRequirePath(lua_State* L) {
  pushRequirePath(L, lovrFilesystemGetRequirePath());
  pushRequirePath(L, lovrFilesystemGetCRequirePath());
  return 2;
}

int l_lovrFilesystemGetSaveDirectory(lua_State* L) {
  lua_pushstring(L, lovrFilesystemGetSaveDirectory());
  return 1;
}

int l_lovrFilesystemGetSize(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  size_t size = lovrFilesystemGetSize(path);
  if ((int) size == -1) {
    return luaL_error(L, "File does not exist");
  }
  lua_pushinteger(L, size);
  return 1;
}

int l_lovrFilesystemGetSource(lua_State* L) {
  const char* source = lovrFilesystemGetSource();

  if (source) {
    lua_pushstring(L, source);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int l_lovrFilesystemGetUserDirectory(lua_State* L) {
  lua_pushstring(L, lovrFilesystemGetUserDirectory());
  return 1;
}

int l_lovrFilesystemGetWorkingDirectory(lua_State* L) {
  char buffer[LOVR_PATH_MAX];

  if (lovrFilesystemGetWorkingDirectory(buffer, sizeof(buffer))) {
    lua_pushnil(L);
  } else {
    lua_pushstring(L, buffer);
  }

  return 1;
}

int l_lovrFilesystemIsDirectory(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, lovrFilesystemIsDirectory(path));
  return 1;
}

int l_lovrFilesystemIsFile(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, lovrFilesystemIsFile(path));
  return 1;
}

int l_lovrFilesystemIsFused(lua_State* L) {
  lua_pushboolean(L, lovrFilesystemIsFused());
  return 1;
}

int l_lovrFilesystemLoad(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  size_t size;
  char* content = lovrFilesystemRead(path, &size);

  if (!content) {
    return luaL_error(L, "Could not read file '%s'", path);
  }

  char debug[LOVR_PATH_MAX];
  snprintf(debug, LOVR_PATH_MAX, "@%s", path);

  int status = luaL_loadbuffer(L, content, size, debug);
  free(content);
  switch (status) {
    case LUA_ERRMEM: return luaL_error(L, "Memory allocation error: %s", lua_tostring(L, -1));
    case LUA_ERRSYNTAX: return luaL_error(L, "Syntax error: %s", lua_tostring(L, -1));
    default: return 1;
  }
}

int l_lovrFilesystemMount(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* mountpoint = luaL_optstring(L, 2, NULL);
  bool append = lua_isnoneornil(L, 3) ? 0 : lua_toboolean(L, 3);
  lua_pushboolean(L, !lovrFilesystemMount(path, mountpoint, append));
  return 1;
}

int l_lovrFilesystemNewBlob(lua_State* L) {
  size_t size;
  const char* path = luaL_checkstring(L, 1);
  uint8_t* data = lovrFilesystemRead(path, &size);
  lovrAssert(data, "Could not load file '%s'", path);
  Blob* blob = lovrBlobCreate((void*) data, size, path);
  luax_pushobject(L, blob);
  lovrRelease(blob);
  return 1;
}

int l_lovrFilesystemRead(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  size_t size;
  char* content = lovrFilesystemRead(path, &size);
  lovrAssert(content, "Could not read file '%s'", path);
  lua_pushlstring(L, content, size);
  free(content);
  return 1;
}

int l_lovrFilesystemReloadAddWatch(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lovrReloadAddWatch(path);
  return 0;
}

int l_lovrFilesystemReloadEnabled(lua_State* L) {
  lua_pushboolean(L, lovrFilesystemReloadEnable);
  return 1;
}

int l_lovrFilesystemReloadCheck(lua_State* L) {
  lua_pushboolean(L, lovrReloadCheck());
  return 1;
}

int l_lovrFilesystemRemove(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, !lovrFilesystemRemove(path));
  return 1;
}

int l_lovrFilesystemSetIdentity(lua_State* L) {
  if (lua_isnoneornil(L, 1)) {
    lovrFilesystemSetIdentity(NULL);
  } else {
    const char* identity = luaL_checkstring(L, 1);
    lovrFilesystemSetIdentity(identity);
  }
  return 0;
}

int l_lovrFilesystemSetRequirePath(lua_State* L) {
  if (lua_type(L, 1) == LUA_TSTRING) lovrFilesystemSetRequirePath(luaL_checkstring(L, 1));
  if (lua_type(L, 2) == LUA_TSTRING) lovrFilesystemSetCRequirePath(luaL_checkstring(L, 2));
  return 0;
}

int l_lovrFilesystemUnmount(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  lua_pushboolean(L, !lovrFilesystemUnmount(path));
  return 1;
}

int l_lovrFilesystemWrite(lua_State* L) {
  size_t size;
  const char* path = luaL_checkstring(L, 1);
  const char* content = luaL_checklstring(L, 2, &size);
  lua_pushnumber(L, lovrFilesystemWrite(path, content, size, 0));
  return 1;
}

const luaL_Reg lovrFilesystem[] = {
  { "append", l_lovrFilesystemAppend },
  { "createDirectory", l_lovrFilesystemCreateDirectory },
  { "getAppdataDirectory", l_lovrFilesystemGetAppdataDirectory },
  { "getDirectoryItems", l_lovrFilesystemGetDirectoryItems },
  { "getExecutablePath", l_lovrFilesystemGetExecutablePath },
  { "getIdentity", l_lovrFilesystemGetIdentity },
  { "getLastModified", l_lovrFilesystemGetLastModified },
  { "getRealDirectory", l_lovrFilesystemGetRealDirectory },
  { "getRequirePath", l_lovrFilesystemGetRequirePath },
  { "getSaveDirectory", l_lovrFilesystemGetSaveDirectory },
  { "getSize", l_lovrFilesystemGetSize },
  { "getSource", l_lovrFilesystemGetSource },
  { "getUserDirectory", l_lovrFilesystemGetUserDirectory },
  { "getWorkingDirectory", l_lovrFilesystemGetWorkingDirectory },
  { "isDirectory", l_lovrFilesystemIsDirectory },
  { "isFile", l_lovrFilesystemIsFile },
  { "isFused", l_lovrFilesystemIsFused },
  { "load", l_lovrFilesystemLoad },
  { "mount", l_lovrFilesystemMount },
  { "newBlob", l_lovrFilesystemNewBlob },
  { "read", l_lovrFilesystemRead },
  { "reloadAddWatch", l_lovrFilesystemReloadAddWatch },
  { "reloadEnabled", l_lovrFilesystemReloadEnabled },
  { "reloadCheck", l_lovrFilesystemReloadCheck },
  { "remove", l_lovrFilesystemRemove },
  { "setRequirePath", l_lovrFilesystemSetRequirePath },
  { "setIdentity", l_lovrFilesystemSetIdentity },
  { "write", l_lovrFilesystemWrite },
  { NULL, NULL }
};
