#include "lib/glfw.h"
#include "headset/headset.h"
#include "oculus_mobile_bridge.h"
#include "math/quat.h"
#include "graphics/graphics.h"
#include "math/mat4.h"
#include "lib/glad/glad.h"
#include <assert.h>
#include "platform.h"

// Data passed from bridge code to headset code

typedef struct {
  BridgeLovrDimensions displayDimensions;
  BridgeLovrDevice deviceType;
  BridgeLovrUpdateData updateData;
} BridgeLovrMobileData;
BridgeLovrMobileData bridgeLovrMobileData;

// Headset

static void (*renderCallback)(void*);
static void* renderUserdata;

static float offset;

// Headset driver object

static bool oculusMobileInit(float _offset, int msaa) {
  // Make sure HeadsetDriver and BridgeLovrDevice have not gone out of sync
  assert(BRIDGE_LOVR_DEVICE_UNKNOWN == HEADSET_UNKNOWN);
  assert(BRIDGE_LOVR_DEVICE_GEAR == HEADSET_GEAR);
  assert(BRIDGE_LOVR_DEVICE_GO == HEADSET_GO);

  offset = _offset;
  return true;
}

static void oculusMobileDestroy() {
}

static HeadsetType oculusMobileGetType() {
  return (HeadsetType)(int)bridgeLovrMobileData.deviceType;
}

static HeadsetOrigin oculusMobileGetOriginType() {
  return ORIGIN_HEAD;
}

static bool oculusMobileIsMounted() {
  return true; // ???
}

static void oculusMobileIsMirrored(bool* mirrored, HeadsetEye * eye) {
  *mirrored = false; // Can't ever??
  *eye = false;
}

static void oculusMobileSetMirrored(bool mirror, HeadsetEye eye) {
}

static void oculusMobileGetDisplayDimensions(uint32_t* width, uint32_t* height) {
  *width = bridgeLovrMobileData.displayDimensions.width;
  *height = bridgeLovrMobileData.displayDimensions.height;
}

static void oculusMobileGetClipDistance(float* clipNear, float* clipFar) {
  // TODO
}

static void oculusMobileSetClipDistance(float clipNear, float clipFar) {
  // TODO
}

static void oculusMobileGetBoundsDimensions(float* width, float* depth) {
  *width = 0;
  *depth = 0;
}

static const float* oculusMobileGetBoundsGeometry(int* count) {
  *count = 0;
  return NULL;
}

static void oculusMobileGetPose(float* x, float* y, float* z, float* angle, float* ax, float* ay, float* az) {
  *x = bridgeLovrMobileData.updateData.lastHeadPose.x;
  *y = bridgeLovrMobileData.updateData.lastHeadPose.y + offset; // Correct for head height
  *z = bridgeLovrMobileData.updateData.lastHeadPose.z;

  // Notice: Ax and Az are both swapped and inverted. Experimentally, if you do the Oculus Go controller position
  // lines up with Lovr visually, and if you don't it doesn't. This is probably needed because of different axis standards.
  quat_getAngleAxis(bridgeLovrMobileData.updateData.lastHeadPose.q, angle, az, ay, ax);
  *ax = -*ax;
  *az = -*az;
}

static void oculusMobileGetEyePose(HeadsetEye eye, float* x, float* y, float* z, float* angle, float* ax, float* ay, float* az) {
  // TODO: This is very wrong!
  oculusMobileGetPose(x, y, z, angle, ax, ay, az);
}

// TODO: This has never been tested
static void oculusMobileGetVelocity(float* x, float* y, float* z) {
  *x = bridgeLovrMobileData.updateData.lastHeadVelocity.x;
  *y = bridgeLovrMobileData.updateData.lastHeadVelocity.y;
  *z = bridgeLovrMobileData.updateData.lastHeadVelocity.z;
}

// TODO: This has never been tested
static void oculusMobileGetAngularVelocity(float* x, float* y, float* z) {
  *x = bridgeLovrMobileData.updateData.lastHeadVelocity.ax;
  *y = bridgeLovrMobileData.updateData.lastHeadVelocity.ay;
  *z = bridgeLovrMobileData.updateData.lastHeadVelocity.az;
}

static Controller *controller;

static Controller** oculusMobileGetControllers(uint8_t* count) {
  if (!controller)
    controller = lovrAlloc(Controller, free);
  *count = bridgeLovrMobileData.updateData.goPresent; // TODO: Figure out what multi controller Oculus Mobile looks like and support it
  return &controller;
}

static bool oculusMobileControllerIsConnected(Controller* controller) {
  return bridgeLovrMobileData.updateData.goPresent;
}

static ControllerHand oculusMobileControllerGetHand(Controller* controller) {
  return HAND_UNKNOWN;
}

static void oculusMobileControllerGetPose(Controller* controller, float* x, float* y, float* z, float* angle, float* ax, float* ay, float* az) {
  *x = bridgeLovrMobileData.updateData.goPose.x;
  *y = bridgeLovrMobileData.updateData.goPose.y + offset; // Correct for head height
  *z = bridgeLovrMobileData.updateData.goPose.z;

  // Notice: Ax and Az are both swapped and inverted. Experimentally, if you do the Oculus Go controller position
  // lines up with Lovr visually, and if you don't it doesn't. This is probably needed because of different axis standards.
  quat_getAngleAxis(bridgeLovrMobileData.updateData.goPose.q, angle, az, ay, ax);
  *ax = -*ax;
  *az = -*az;
}

static float oculusMobileControllerGetAxis(Controller* controller, ControllerAxis axis) {
  switch (axis) {
    case CONTROLLER_AXIS_TOUCHPAD_X:
      return (bridgeLovrMobileData.updateData.goTrackpad.x-160)/160.0;
    case CONTROLLER_AXIS_TOUCHPAD_Y:
      return (bridgeLovrMobileData.updateData.goTrackpad.y-160)/160.0;
    case CONTROLLER_AXIS_TRIGGER:
      return bridgeLovrMobileData.updateData.goButtonDown ? 1.0 : 0.0;
    default:
      return 0;
  }
}

static bool buttonCheck(BridgeLovrButton field, ControllerButton button) {
  switch (button) {
    case CONTROLLER_BUTTON_MENU:
      return field & BRIDGE_LOVR_BUTTON_MENU;
    case CONTROLLER_BUTTON_TRIGGER:
      return field & BRIDGE_LOVR_BUTTON_SHOULDER;
    case CONTROLLER_BUTTON_TOUCHPAD:
      return field & BRIDGE_LOVR_BUTTON_TOUCHPAD;
    default:
      return false;
  }
}

static bool oculusMobileControllerIsDown(Controller* controller, ControllerButton button) {
  return buttonCheck(bridgeLovrMobileData.updateData.goButtonDown, button);
}

static bool oculusMobileControllerIsTouched(Controller* controller, ControllerButton button) {
  return buttonCheck(bridgeLovrMobileData.updateData.goButtonTouch, button);
}

static void oculusMobileControllerVibrate(Controller* controller, float duration, float power) {
}

static ModelData* oculusMobileControllerNewModelData(Controller* controller) {
  return NULL;
}

// TODO: need to set up swap chain textures for the eyes and finish view transforms
static void oculusMobileRenderTo(void (*callback)(void*), void* userdata) {
  renderCallback = callback;
  renderUserdata = userdata;
}

static void oculusMobileUpdate(float dt) {
}

HeadsetInterface lovrHeadsetOculusMobileDriver = {
  DRIVER_OCULUS_MOBILE,
  oculusMobileInit,
  oculusMobileDestroy,
  oculusMobileGetType,
  oculusMobileGetOriginType,
  oculusMobileIsMounted,
  oculusMobileIsMirrored,
  oculusMobileSetMirrored,
  oculusMobileGetDisplayDimensions,
  oculusMobileGetClipDistance,
  oculusMobileSetClipDistance,
  oculusMobileGetBoundsDimensions,
  oculusMobileGetBoundsGeometry,
  oculusMobileGetPose,
  oculusMobileGetEyePose,
  oculusMobileGetVelocity,
  oculusMobileGetAngularVelocity,
  oculusMobileGetControllers,
  oculusMobileControllerIsConnected,
  oculusMobileControllerGetHand,
  oculusMobileControllerGetPose,
  oculusMobileControllerGetAxis,
  oculusMobileControllerIsDown,
  oculusMobileControllerIsTouched,
  oculusMobileControllerVibrate,
  oculusMobileControllerNewModelData,
  oculusMobileRenderTo,
  oculusMobileUpdate
};

// Pseudo GLFW

void glfwPollEvents() {

}

GLFWAPI void glfwGetCursorPos(GLFWwindow* window, double* xpos, double* ypos) {
  *xpos = 0;
  *ypos = 0;
}

GLFWwindow* glfwGetCurrentContext(void) {
  return NULL;
}

static double timeOffset;

void glfwSetTime(double time) {
  timeOffset = bridgeLovrMobileData.updateData.displayTime - time;
}
double glfwGetTime(void) {
  return bridgeLovrMobileData.updateData.displayTime - timeOffset;
}

static GLFWerrorfun lastErrFun;
GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun cbfun)
{
  GLFWerrorfun cb = lastErrFun;
  lastErrFun = cbfun;
  return cb;
}

int glfwInit(void) {
  return 1;
}
void glfwTerminate(void) {

}

GLFWAPI void glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height)
{
  *width = bridgeLovrMobileData.displayDimensions.width;
  *height = bridgeLovrMobileData.displayDimensions.height; 
}

// "Bridge" (see oculus_mobile_bridge.h)

#include <stdio.h>
#include <android/log.h>
#include "physfs.h"
#include <sys/stat.h>
#include <assert.h>

#include "oculus_mobile_bridge.h"
#include "luax.h"
#include "lib/sds/sds.h"
#include "lib/glfw.h"

#include "api.h"
#include "lib/lua-cjson/lua_cjson.h"
#include "lib/lua-enet/enet.h"
#include "headset/oculus_mobile.h"

// Implicit from boot.lua.h
extern unsigned char boot_lua[];
extern unsigned int boot_lua_len;

static lua_State *L, *Lcoroutine;
static int coroutineRef = LUA_NOREF;
static int coroutineStartFunctionRef = LUA_NOREF;

// Expose to filesystem.h
char *lovrOculusMobileWritablePath;

// Used for resume (pausing the app and returning to the menu) logic. This is needed for two reasons
// 1. The GLFW time should rewind after a pause so that the app cannot perceive time passed
// 2. There is a bug in the Mobile SDK https://developer.oculus.com/bugs/bug/189155031962759/
//    On the first frame after a resume, the time will be total nonsense
static double lastPauseAt, lastPauseAtRaw; // glfw time and oculus time at last pause
enum {
  PAUSESTATE_NONE,   // Normal state
  PAUSESTATE_PAUSED, // A pause has been issued -- waiting for resume
  PAUSESTATE_BUG,    // We have resumed, but the next frame will be the bad frame
  PAUSESTATE_RESUME  // We have resumed, and the next frame will need to adjust the clock
} pauseState;

int lovr_luaB_print_override (lua_State *L);

#define SDS(...) sdscatfmt(sdsempty(), __VA_ARGS__)

// Recursively copy a subdirectory out of PhysFS onto disk
static void physCopyFiles(sds toDir, sds fromDir) {
  char **filesOrig = PHYSFS_enumerateFiles(fromDir);
  char **files = filesOrig;

  if (!files) {
    __android_log_print(ANDROID_LOG_ERROR, "LOVR", "COULD NOT READ DIRECTORY SOMEHOW: [%s]", fromDir);
    return;
  }

  mkdir(toDir, 0777);

  while (*files) {
    sds fromPath = SDS("%S/%s", fromDir, *files);
    sds toPath = SDS("%S/%s", toDir, *files);
    PHYSFS_Stat stat;
    PHYSFS_stat(fromPath, &stat);

    if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
      __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "DIR:  [%s] INTO: [%s]", fromPath, toPath);
      physCopyFiles(toPath, fromPath);
    } else {
      __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "FILE: [%s] INTO: [%s]", fromPath, toPath);

      PHYSFS_File *fromFile = PHYSFS_openRead(fromPath);

      if (!fromFile) {
        __android_log_print(ANDROID_LOG_ERROR, "LOVR", "COULD NOT OPEN TO READ:  [%s]", fromPath);
      
      } else {
        FILE *toFile = fopen(toPath, "w");

        if (!toFile) {
          __android_log_print(ANDROID_LOG_ERROR, "LOVR", "COULD NOT OPEN TO WRITE: [%s]", toPath);

        } else {
#define CPBUFSIZE (1024*8)
          while(1) {
            char buffer[CPBUFSIZE];
            int written = PHYSFS_readBytes(fromFile, buffer, CPBUFSIZE);
            if (written > 0)
              fwrite(buffer, 1, written, toFile);
            if (PHYSFS_eof(fromFile))
              break;
          }
          fclose(toFile);
        }
        PHYSFS_close(fromFile);
      }
    }
    files++;
    sdsfree(fromPath);
    sdsfree(toPath);
  }
  PHYSFS_freeList(filesOrig);
}

static void android_vthrow(lua_State* L, const char* format, va_list args) {
  #define MAX_ERROR_LENGTH 1024
  char lovrErrorMessage[MAX_ERROR_LENGTH];
  vsnprintf(lovrErrorMessage, MAX_ERROR_LENGTH, format, args);
  __android_log_print(ANDROID_LOG_FATAL, "LOVR", "Error: %s\n", lovrErrorMessage);
  assert(0);
}

static int luax_preloadmodule(lua_State* L, const char* key, lua_CFunction f) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, f);
  lua_setfield(L, -2, key);
  lua_pop(L, 2);
  return 0;
}

static int luax_custom_atpanic(lua_State *L)
{
    const char *msg = lua_tostring(L, -1);
    // This doesn't appear to get a sensible stack. Maybe Luajit would work better?
    if (luax_getstack_panic(L)) {
      msg = lua_tostring(L, -1);
    }
    lovrThrow("Lua panic: %s", msg);
    return 0;
}

void bridgeLovrInit(BridgeLovrInitData *initData) {
  __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "\n INSIDE LOVR\n");

  // Save writable data directory for LovrFilesystemInit later
  {
    lovrOculusMobileWritablePath = sdsRemoveFreeSpace(SDS("%s/data", initData->writablePath));
    mkdir(lovrOculusMobileWritablePath, 0777);
  }

  // This is a bit fancy. We want to run files off disk instead of out of the zip file.
  // This is for two reasons: Because PhysFS won't let us mount "within" a zip;
  // and because if we run the files out of a temp data directory, we can overwrite them
  // with "adb push" to debug.
  // As a TODO, when PHYSFS_mountSubdir lands, this path should change to only run in debug mode.
  sds programPath = SDS("%s/program", initData->writablePath);
  {
    // We will store the last apk change time in this "lastprogram" file.
    // We will copy all the files out of the zip into the temp dir, but ONLY if they've changed.
    sds timePath = SDS("%s/lastprogram.dat", initData->writablePath);

    // When did APK last change?
    struct stat apkstat;
    int statfail = stat(initData->apkPath, &apkstat);
    if (statfail) {
      __android_log_print(ANDROID_LOG_ERROR, "LOVR", "CAN'T FIND APK [%s]\n", initData->apkPath);
      assert(0);
    }

    // When did we last do a file copy?
    struct timespec previoussec;
    FILE *timeFile = fopen(timePath, "r");
    bool copyFiles = !timeFile; // If no lastprogram.dat, we've never copied
    if (timeFile) {
      fread(&previoussec.tv_sec, sizeof(previoussec.tv_sec), 1, timeFile);
      fread(&previoussec.tv_nsec, sizeof(previoussec.tv_nsec), 1, timeFile);
      fclose(timeFile);

      copyFiles = apkstat.st_mtim.tv_sec != previoussec.tv_sec || // If timestamp differs, apk changed
               apkstat.st_mtim.tv_nsec != previoussec.tv_nsec;
    }

    if (copyFiles) {
      __android_log_print(ANDROID_LOG_ERROR, "LOVR", "APK CHANGED [%s] WILL UNPACK\n", initData->apkPath);

      // PhysFS hasn't been inited, so we can temporarily use it as an unzip utility if we deinit afterward
      PHYSFS_init("lovr");
      int success = PHYSFS_mount(initData->apkPath, NULL, 1);
      if (!success) {
        __android_log_print(ANDROID_LOG_ERROR, "LOVR", "FAILED TO MOUNT APK [%s]\n", initData->apkPath);
        assert(0);
      } else {
        sds rootFromDir = sdsnew("/assets");
        physCopyFiles(programPath, rootFromDir);
        sdsfree(rootFromDir);
      }
      PHYSFS_deinit();

      // Save timestamp in a new lastprogram.dat file
      timeFile = fopen(timePath, "w");
      fwrite(&apkstat.st_mtim.tv_sec, sizeof(apkstat.st_mtim.tv_sec), 1, timeFile);
      fwrite(&apkstat.st_mtim.tv_nsec, sizeof(apkstat.st_mtim.tv_nsec), 1, timeFile);
      fclose(timeFile);
    }

    sdsfree(timePath);
  }

  // Unpack init data
  bridgeLovrMobileData.displayDimensions = initData->suggestedEyeTexture;
  bridgeLovrMobileData.updateData.displayTime = initData->zeroDisplayTime;
  bridgeLovrMobileData.deviceType = initData->deviceType;

  // Ready to actually go now.
  // Copypaste the init sequence from lovrRun:
  // Load libraries
  L = luaL_newstate(); // FIXME: Just call main?
  luax_setmainstate(L);
  lua_atpanic(L, luax_custom_atpanic);
  luaL_openlibs(L);
  __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "\n OPENED LIB\n");

  lovrSetErrorCallback((lovrErrorHandler) android_vthrow, L);

  // Install custom print
  static const struct luaL_Reg printHack [] = {
    {"print", lovr_luaB_print_override},
    {NULL, NULL} /* end of array */
  };
  lua_getglobal(L, "_G");
  luaL_register(L, NULL, printHack); // "for Lua versions < 5.2"
  //luaL_setfuncs(L, printlib, 0);  // "for Lua versions 5.2 or greater"
  lua_pop(L, 1);

  glfwSetTime(0);

  // Set "arg" global
  {
    const char *argv[] = {"lovr", programPath};
    int argc = 2;

    lua_newtable(L);
    lua_pushstring(L, "lovr");
    lua_rawseti(L, -2, -1);
    for (int i = 0; i < argc; i++) {
      lua_pushstring(L, argv[i]);
      lua_rawseti(L, -2, i == 0 ? -2 : i);
    }
    lua_setglobal(L, "arg");
  }

  sdsfree(programPath);

  // Register loaders for internal packages (since dynamic load does not seem to work on Android)
  luax_preloadmodule(L, "lovr", luaopen_lovr);
  luax_preloadmodule(L, "lovr.audio", luaopen_lovr_audio);
  luax_preloadmodule(L, "lovr.data", luaopen_lovr_data);
  luax_preloadmodule(L, "lovr.event", luaopen_lovr_event);
  luax_preloadmodule(L, "lovr.filesystem", luaopen_lovr_filesystem);
  luax_preloadmodule(L, "lovr.graphics", luaopen_lovr_graphics);
  luax_preloadmodule(L, "lovr.headset", luaopen_lovr_headset);
  luax_preloadmodule(L, "lovr.math", luaopen_lovr_math);
  luax_preloadmodule(L, "lovr.physics", luaopen_lovr_physics);
  luax_preloadmodule(L, "lovr.thread", luaopen_lovr_thread);
  luax_preloadmodule(L, "lovr.timer", luaopen_lovr_timer);
  luax_preloadmodule(L, "cjson", luaopen_cjson);
  luax_preloadmodule(L, "enet", luaopen_enet);

  // Run init

  lua_pushcfunction(L, luax_getstack);
  if (luaL_loadbuffer(L, (const char*) boot_lua, boot_lua_len, "boot.lua") || lua_pcall(L, 0, 1, -2)) {
    __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "\n LUA STARTUP FAILED: %s\n", lua_tostring(L, -1));
    lua_close(L);
    assert(0);
  }

  coroutineStartFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX); // Value returned by boot.lua
  Lcoroutine = lua_newthread(L); // Leave L clear to be used by the draw function
  lua_atpanic(Lcoroutine, luax_custom_atpanic);
  coroutineRef = luaL_ref(L, LUA_REGISTRYINDEX); // Hold on to the Lua-side coroutine object so it isn't GC'd

  __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "\n BRIDGE INIT COMPLETE top %d\n", (int)lua_gettop(L));
}

void bridgeLovrUpdate(BridgeLovrUpdateData *updateData) {
  // Unpack update data
  bridgeLovrMobileData.updateData = *updateData;

  if (pauseState == PAUSESTATE_BUG) { // Bad frame-- replace bad time with last known good oculus time
    bridgeLovrMobileData.updateData.displayTime = lastPauseAtRaw;
    pauseState = PAUSESTATE_RESUME;
  } else if (pauseState == PAUSESTATE_RESUME) { // Resume frame-- adjust glfw time to be equal to last good glfw time
    glfwSetTime(lastPauseAt);
    pauseState = PAUSESTATE_NONE;
  }

  // Go
  if (coroutineStartFunctionRef != LUA_NOREF) {
    lua_rawgeti(Lcoroutine, LUA_REGISTRYINDEX, coroutineStartFunctionRef);
    luaL_unref (Lcoroutine, LUA_REGISTRYINDEX, coroutineStartFunctionRef);
    coroutineStartFunctionRef = LUA_NOREF; // No longer needed
  }
  int coroutineArgs = luax_pushLovrHeadsetRenderError(Lcoroutine);
  if (lua_resume(Lcoroutine, coroutineArgs) != LUA_YIELD) {
    __android_log_print(ANDROID_LOG_DEBUG, "LOVR", "\n LUA QUIT\n");
    assert(0);
  }
}

static void lovrOculusMobileDraw(int framebuffer, int width, int height, float *eyeViewMatrix, float *projectionMatrix) {
  lovrGpuDirtyTexture();

  CanvasFlags flags = {0};
  Canvas *canvas = lovrCanvasCreateFromHandle(width, height, flags, framebuffer, 0, 0, 1, true);

  Camera camera = { .canvas = canvas, .stereo = false };
  memcpy(camera.viewMatrix[0], eyeViewMatrix, sizeof(camera.viewMatrix[0]));
  mat4_translate(camera.viewMatrix[0], 0, -offset, 0);

  memcpy(camera.projection[0], projectionMatrix, sizeof(camera.projection[0]));

  lovrGraphicsSetCamera(&camera, true);

  if (renderCallback)
    renderCallback(renderUserdata);

  lovrGraphicsSetCamera(NULL, false);
  lovrRelease(canvas);
}

void bridgeLovrDraw(BridgeLovrDrawData *drawData) {
  int eye = drawData->eye;
  lovrOculusMobileDraw(drawData->framebuffer, bridgeLovrMobileData.displayDimensions.width, bridgeLovrMobileData.displayDimensions.height,
    bridgeLovrMobileData.updateData.eyeViewMatrix[eye], bridgeLovrMobileData.updateData.projectionMatrix[eye]); // Is this indexing safe?
}

// Android activity has been stopped or resumed
// In order to prevent weird dt jumps, we need to freeze and reset the clock
static bool armedUnpause;
void bridgeLovrPaused(bool paused) {
  if (paused) { // Save last glfw and oculus times and wait for resume
    lastPauseAt = glfwGetTime();
    lastPauseAtRaw = bridgeLovrMobileData.updateData.displayTime;
    pauseState = PAUSESTATE_PAUSED;
  } else {
    if (pauseState != PAUSESTATE_NONE) { // Got a resume-- set flag to start the state machine in bridgeLovrUpdate
      pauseState = PAUSESTATE_BUG;
    }
  }
}

// Android activity has been "destroyed" (but process will probably not quit)
void bridgeLovrClose() {
  pauseState = PAUSESTATE_NONE;
  lua_close(L);
}
