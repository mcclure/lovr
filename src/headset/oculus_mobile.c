#include "headset/headset.h"
#include "oculus_mobile_bridge.h"
#include "math.h"
#include "graphics/graphics.h"
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

static vec_controller_t controllers;

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
  assert(BRIDGE_LOVR_DEVICE_QUEST == HEADSET_QUEST);

  offset = _offset;
  vec_init(&controllers);

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
  return true;
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
  *width = 0.f;
  *depth = 0.f;
}

static const float* oculusMobileGetBoundsGeometry(int* count) {
  *count = 0;
  return NULL;
}

static void oculusMobileGetPose(float* x, float* y, float* z, float* angle, float* ax, float* ay, float* az) {
  *x = bridgeLovrMobileData.updateData.lastHeadPose.x;
  *y = bridgeLovrMobileData.updateData.lastHeadPose.y + offset; // Correct for head height
  *z = bridgeLovrMobileData.updateData.lastHeadPose.z;
  quat_getAngleAxis(bridgeLovrMobileData.updateData.lastHeadPose.q, angle, ax, ay, az);
}

static void oculusMobileGetVelocity(float* vx, float* vy, float* vz) {
  *vx = bridgeLovrMobileData.updateData.lastHeadVelocity.x;
  *vy = bridgeLovrMobileData.updateData.lastHeadVelocity.y;
  *vz = bridgeLovrMobileData.updateData.lastHeadVelocity.z;
}

static void oculusMobileGetAngularVelocity(float* vx, float* vy, float* vz) {
  *vx = bridgeLovrMobileData.updateData.lastHeadVelocity.ax;
  *vy = bridgeLovrMobileData.updateData.lastHeadVelocity.ay;
  *vz = bridgeLovrMobileData.updateData.lastHeadVelocity.az;
}

static Controller** oculusMobileGetControllers(uint8_t* count) {
  *count = controllers.length;
  return controllers.data;
}

// TODO: Currently controllers are 1:1 matched to id and hand is assumed based on id.
// It's unclear what this does if controllers start appearing and disappearing.
static bool oculusMobileControllerIsConnected(Controller* controller) {
  return controller->id < controllers.length;
}

static ControllerHand oculusMobileControllerGetHand(Controller* controller) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];

    if (data->hand & BRIDGE_LOVR_HAND_LEFT) {
      return HAND_LEFT;
    } else {
      return HAND_RIGHT;
    }
  }
  return HAND_UNKNOWN;
}

static void oculusMobileControllerGetPose(Controller* controller, float* x, float* y, float* z, float* angle, float* ax, float* ay, float* az) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];

    *x = data->pose.x;
    *y = data->pose.y + offset; // Correct for head height
    *z = data->pose.z;
    quat_getAngleAxis(data->pose.q, angle, ax, ay, az);
  } else {
    *x = *y = *z = *angle = *az = *ay = *ax = 0;
  }
}

static void oculusMobileControllerGetVelocity(Controller* controller, float* vx, float* vy, float* vz) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];
    
    *vx = data->velocity.x;
    *vy = data->velocity.y;
    *vz = data->velocity.z;
  } else {
    *vx = *vy = *vz = 0;
  }
}

static void oculusMobileControllerGetAngularVelocity(Controller* controller, float* vx, float* vy, float* vz) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];
    
    *vx = data->velocity.ax;
    *vy = data->velocity.ay;
    *vz = data->velocity.az;
  } else {
    *vx = *vy = *vz = 0;
  }
}

static bool buttonDown(BridgeLovrButton field, ControllerButton button) {
  if (bridgeLovrMobileData.deviceType == BRIDGE_LOVR_DEVICE_QUEST) {
    switch (button) {
      case CONTROLLER_BUTTON_MENU: return field & BRIDGE_LOVR_BUTTON_MENU; // Technically "LMENU" but only fires on left controller
      case CONTROLLER_BUTTON_TRIGGER: return field & BRIDGE_LOVR_BUTTON_SHOULDER;
      case CONTROLLER_BUTTON_GRIP: return field & BRIDGE_LOVR_BUTTON_GRIP;
      case CONTROLLER_BUTTON_TOUCHPAD: return field & BRIDGE_LOVR_BUTTON_JOYSTICK;
      case CONTROLLER_BUTTON_A: return field & BRIDGE_LOVR_BUTTON_A;
      case CONTROLLER_BUTTON_B: return field & BRIDGE_LOVR_BUTTON_B;
      case CONTROLLER_BUTTON_X: return field & BRIDGE_LOVR_BUTTON_X;
      case CONTROLLER_BUTTON_Y: return field & BRIDGE_LOVR_BUTTON_Y;
      default: return false;
    }
  } else {
    switch (button) {
      case CONTROLLER_BUTTON_MENU: return field & BRIDGE_LOVR_BUTTON_GOMENU; // Technically "RMENU" but quest only has one
      case CONTROLLER_BUTTON_TRIGGER: return field & BRIDGE_LOVR_BUTTON_GOSHOULDER;
      case CONTROLLER_BUTTON_TOUCHPAD: return field & BRIDGE_LOVR_BUTTON_TOUCHPAD;
      default: return false;
    }
  }
}

static bool buttonTouch(BridgeLovrTouch field, ControllerButton button) {
  switch (button) {
    case CONTROLLER_BUTTON_TRIGGER: return field & BRIDGE_LOVR_TOUCH_TRIGGER;
    //case CONTROLLER_BUTTON_GRIP: return field & BRIDGE_LOVR_BUTTON_GRIP;
    case CONTROLLER_BUTTON_TOUCHPAD: return field & (BRIDGE_LOVR_TOUCH_TOUCHPAD | BRIDGE_LOVR_TOUCH_JOYSTICK);
    case CONTROLLER_BUTTON_A: return field & BRIDGE_LOVR_TOUCH_A;
    case CONTROLLER_BUTTON_B: return field & BRIDGE_LOVR_TOUCH_B;
    case CONTROLLER_BUTTON_X: return field & BRIDGE_LOVR_TOUCH_X;
    case CONTROLLER_BUTTON_Y: return field & BRIDGE_LOVR_TOUCH_Y;
    default: return false;
  }
}


static float oculusMobileControllerGetAxis(Controller* controller, ControllerAxis axis) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];
    
    if (bridgeLovrMobileData.deviceType == BRIDGE_LOVR_DEVICE_QUEST) {
      switch (axis) {
        case CONTROLLER_AXIS_TOUCHPAD_X:
          return data->trackpad.x;
        case CONTROLLER_AXIS_TOUCHPAD_Y:
          return data->trackpad.y;
        case CONTROLLER_AXIS_TRIGGER:
          return data->trigger;
        case CONTROLLER_AXIS_GRIP:
          return data->grip;
        default:
          return 0;
      }
    } else {
      switch (axis) {
        case CONTROLLER_AXIS_TOUCHPAD_X:
          return (data->trackpad.x - 160) / 160.f;
        case CONTROLLER_AXIS_TOUCHPAD_Y:
          return (data->trackpad.y- 160 ) / 160.f;
        case CONTROLLER_AXIS_TRIGGER:
          return buttonDown(data->buttonDown, CONTROLLER_BUTTON_TRIGGER) ? 1.f : 0.f;
        default:
          return 0;
      }
    }
  } else {
    return 0;
  }
}

static bool oculusMobileControllerIsDown(Controller* controller, ControllerButton button) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];
    
    return buttonDown(data->buttonDown, button);
  } else {
    return false;
  }
}

static bool oculusMobileControllerIsTouched(Controller* controller, ControllerButton button) {
  if (controller->id < bridgeLovrMobileData.updateData.controllerCount) {
    BridgeLovrController *data = &bridgeLovrMobileData.updateData.controllers[controller->id];
    
    return buttonTouch(data->buttonTouch, button);
  } else {
    return false;
  }
}

static void oculusMobileControllerVibrate(Controller* controller, float duration, float power) {
  //
}

static ModelData* oculusMobileControllerNewModelData(Controller* controller) {
  return NULL;
}

// TODO: need to set up swap chain textures for the eyes and finish view transforms
static void oculusMobileRenderTo(void (*callback)(void*), void* userdata) {
  renderCallback = callback;
  renderUserdata = userdata;
}

HeadsetInterface lovrHeadsetOculusMobileDriver = {
  DRIVER_OCULUS_MOBILE,
  oculusMobileInit,
  oculusMobileDestroy,
  oculusMobileGetType,
  oculusMobileGetOriginType,
  oculusMobileIsMounted,
  oculusMobileGetDisplayDimensions,
  oculusMobileGetClipDistance,
  oculusMobileSetClipDistance,
  oculusMobileGetBoundsDimensions,
  oculusMobileGetBoundsGeometry,
  oculusMobileGetPose,
  oculusMobileGetVelocity,
  oculusMobileGetAngularVelocity,
  oculusMobileGetControllers,
  oculusMobileControllerIsConnected,
  oculusMobileControllerGetHand,
  oculusMobileControllerGetPose,
  oculusMobileControllerGetVelocity,
  oculusMobileControllerGetAngularVelocity,
  oculusMobileControllerGetAxis,
  oculusMobileControllerIsDown,
  oculusMobileControllerIsTouched,
  oculusMobileControllerVibrate,
  oculusMobileControllerNewModelData,
  oculusMobileRenderTo,
  .getMirrorTexture = NULL,
  .update = NULL
};

// Oculus-specific platform functions

static double timeOffset;

void lovrPlatformSetTime(double time) {
  timeOffset = bridgeLovrMobileData.updateData.displayTime - time;
}

double lovrPlatformGetTime(void) {
  return bridgeLovrMobileData.updateData.displayTime - timeOffset;
}

void lovrPlatformGetFramebufferSize(int* width, int* height) {
  if (width)
    *width = bridgeLovrMobileData.displayDimensions.width;
  if (height)
    *height = bridgeLovrMobileData.displayDimensions.height;
}

bool lovrPlatformHasWindow() {
  return false;
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

#include "api.h"
#include "lib/lua-cjson/lua_cjson.h"
#include "lib/lua-enet/enet.h"
#include "headset/oculus_mobile.h"

// Implicit from boot.lua.h
extern unsigned char boot_lua[];
extern unsigned int boot_lua_len;

static lua_State *L, *T;
static int coroutineRef = LUA_NOREF;
static int coroutineStartFunctionRef = LUA_NOREF;

static char *apkPath;

// Expose to filesystem.h
char *lovrOculusMobileWritablePath;

// Used for resume (pausing the app and returning to the menu) logic. This is needed for two reasons
// 1. The GLFW time should rewind after a pause so that the app cannot perceive time passed
// 2. There is a bug in the Mobile SDK https://developer.oculus.com/bugs/bug/189155031962759/
//    On the first frame after a resume, the time will be total nonsense
static double lastPauseAt, lastPauseAtRaw; // platform time and oculus time at last pause
enum {
  PAUSESTATE_NONE,   // Normal state
  PAUSESTATE_PAUSED, // A pause has been issued -- waiting for resume
  PAUSESTATE_BUG,    // We have resumed, but the next frame will be the bad frame
  PAUSESTATE_RESUME  // We have resumed, and the next frame will need to adjust the clock
} pauseState;

int lovr_luaB_print_override (lua_State *L);

#define SDS(...) sdscatfmt(sdsempty(), __VA_ARGS__)

static void android_vthrow(lua_State* L, const char* format, va_list args) {
  #define MAX_ERROR_LENGTH 1024
  char lovrErrorMessage[MAX_ERROR_LENGTH];
  vsnprintf(lovrErrorMessage, MAX_ERROR_LENGTH, format, args);
  lovrWarn("Error: %s\n", lovrErrorMessage);
  assert(0);
}

static int luax_custom_atpanic(lua_State *L) {
  // This doesn't appear to get a sensible stack. Maybe Luajit would work better?
  luax_traceback(L, L, lua_tostring(L, -1), 0); // Pushes the traceback onto the stack
  lovrThrow("Lua panic: %s", lua_tostring(L, -1));
  return 0;
}

static void bridgeLovrInitState() {
  // Ready to actually go now.
  // Copypaste the init sequence from lovrRun:
  // Load libraries
  L = luaL_newstate(); // FIXME: Can this be handed off to main.c?
  luax_setmainthread(L);
  lua_atpanic(L, luax_custom_atpanic);
  luaL_openlibs(L);
  lovrLog("\n OPENED LIB\n");

  lovrSetErrorCallback((lovrErrorHandler) android_vthrow, L);

  // Install custom print
  lua_pushcfunction(L, luax_print);
  lua_setglobal(L, "print");

  lovrPlatformSetTime(0);

  // Set "arg" global (see main.c)
  {
    lua_newtable(L);
    lua_pushliteral(L, "lovr");
    lua_pushvalue(L, -1); // Double at named key
    lua_setfield(L, -3, "exe");
    lua_rawseti(L, -2, -3);

    // Mimic the arguments "--root /assets" as parsed by lovrInit
    lua_pushliteral(L, "--root");
    lua_rawseti(L, -2, -2);
    lua_pushliteral(L, "/assets");
    lua_pushvalue(L, -1); // Double at named key
    lua_setfield(L, -3, "root");
    lua_rawseti(L, -2, -1);

    lua_pushstring(L, apkPath);
    lua_rawseti(L, -2, 0);

    lua_setglobal(L, "arg");
  }

  // Populate package.preload with built-in modules
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  luaL_register(L, NULL, lovrModules);
  lua_pop(L, 2);

  // Run init

  lua_pushcfunction(L, luax_getstack);
  if (luaL_loadbuffer(L, (const char*) boot_lua, boot_lua_len, "boot.lua") || lua_pcall(L, 0, 1, -2)) {
    lovrWarn("\n LUA STARTUP FAILED: %s\n", lua_tostring(L, -1));
    lua_close(L);
    assert(0);
  }

  coroutineStartFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX); // Value returned by boot.lua
  T = lua_newthread(L); // Leave L clear to be used by the draw function
  lua_atpanic(T, luax_custom_atpanic);
  coroutineRef = luaL_ref(L, LUA_REGISTRYINDEX); // Hold on to the Lua-side coroutine object so it isn't GC'd

  lovrLog("\n STATE INIT COMPLETE\n");
}

void bridgeManageControllers(BridgeLovrUpdateData *updateData) {
  if (updateData->controllerCount != controllers.length) {
    Controller *controller; int idx;
    vec_foreach(&controllers, controller, idx) {
      lovrRelease(controller);
    }
    vec_clear(&controllers);

    for (idx = 0; idx < updateData->controllerCount; idx++) {
      controller = lovrAlloc(Controller);
      controller->id = idx;
      vec_push(&controllers, controller);
    }
  }
}

void bridgeLovrInit(BridgeLovrInitData *initData) {
  lovrLog("\n INSIDE LOVR\n");

  // Save writable data directory for LovrFilesystemInit later
  {
    lovrOculusMobileWritablePath = sdsRemoveFreeSpace(SDS("%s/data", initData->writablePath));
    mkdir(lovrOculusMobileWritablePath, 0777);
  }

  // Unpack init data
  bridgeLovrMobileData.displayDimensions = initData->suggestedEyeTexture;
  bridgeLovrMobileData.updateData.displayTime = initData->zeroDisplayTime;
  bridgeLovrMobileData.deviceType = initData->deviceType;

  free(apkPath);
  apkPath = strdup(initData->apkPath);

  bridgeLovrInitState();

  lovrLog("\n BRIDGE INIT COMPLETE\n");
}

void bridgeLovrUpdate(BridgeLovrUpdateData *updateData) {
  // Unpack update data
  bridgeLovrMobileData.updateData = *updateData;
  bridgeManageControllers(updateData);

//  for(int c = 0; c < updateData->controllerCount; c++) lovrLog("%d: d %x t %x\n", c, (uint32_t)updateData->controllers[c].buttonDown, (uint32_t)updateData->controllers[c].buttonTouch);

  if (pauseState == PAUSESTATE_BUG) { // Bad frame-- replace bad time with last known good oculus time
    bridgeLovrMobileData.updateData.displayTime = lastPauseAtRaw;
    pauseState = PAUSESTATE_RESUME;
  } else if (pauseState == PAUSESTATE_RESUME) { // Resume frame-- adjust platform time to be equal to last good platform time
    lovrPlatformSetTime(lastPauseAt);
    pauseState = PAUSESTATE_NONE;
  }

  // Go
  if (coroutineStartFunctionRef != LUA_NOREF) {
    lua_rawgeti(T, LUA_REGISTRYINDEX, coroutineStartFunctionRef);
    luaL_unref (T, LUA_REGISTRYINDEX, coroutineStartFunctionRef);
    coroutineStartFunctionRef = LUA_NOREF; // No longer needed
  }

  luax_geterror(T);
  luax_clearerror(T);
  if (lua_resume(T, 1) != LUA_YIELD) {
    if (lua_type(T, -1) == LUA_TSTRING && !strcmp(lua_tostring(T, -1), "restart")) {
      lua_close(L);
      bridgeLovrInitState();
    } else {
      lovrLog("\n LUA REQUESTED A QUIT\n");
      assert(0);
    }
  }
}

static void lovrOculusMobileDraw(int framebuffer, bool multiview, int width, int height, float *eyeViewMatrix, float *projectionMatrix) {
  lovrGpuDirtyTexture();

  Canvas canvas = { 0 };
  CanvasFlags flags = { .multiview = multiview };
  lovrCanvasInitFromHandle(&canvas, width, height, flags, framebuffer, 0, 0, 1, true);

  Camera camera = { .canvas = &canvas, .stereo = false };

  if (multiview) {
    mat4_init(camera.viewMatrix[0], bridgeLovrMobileData.updateData.eyeViewMatrix[0]);
    mat4_init(camera.viewMatrix[1], bridgeLovrMobileData.updateData.eyeViewMatrix[1]);
    mat4_init(camera.projection[0], bridgeLovrMobileData.updateData.projectionMatrix[0]);
    mat4_init(camera.projection[1], bridgeLovrMobileData.updateData.projectionMatrix[1]);
    mat4_translate(camera.viewMatrix[0], 0, -offset, 0);
    mat4_translate(camera.viewMatrix[1], 0, -offset, 0);
  } else {
    mat4_init(camera.viewMatrix[0], eyeViewMatrix);
    mat4_init(camera.projection[0], projectionMatrix);
    mat4_translate(camera.viewMatrix[0], 0, -offset, 0);
  }

  lovrGraphicsSetCamera(&camera, true);

  if (renderCallback) {
    renderCallback(renderUserdata);
  }

  lovrGraphicsSetCamera(NULL, false);
  lovrCanvasDestroy(&canvas);
}

void bridgeLovrDraw(BridgeLovrDrawData *drawData) {
  int eye = drawData->eye;
  lovrOculusMobileDraw(drawData->framebuffer, drawData->multiview, bridgeLovrMobileData.displayDimensions.width, bridgeLovrMobileData.displayDimensions.height,
    bridgeLovrMobileData.updateData.eyeViewMatrix[eye], bridgeLovrMobileData.updateData.projectionMatrix[eye]); // Is this indexing safe?
}

// Android activity has been stopped or resumed
// In order to prevent weird dt jumps, we need to freeze and reset the clock
static bool armedUnpause;
void bridgeLovrPaused(bool paused) {
  if (paused) { // Save last platform and oculus times and wait for resume
    lastPauseAt = lovrPlatformGetTime();
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
