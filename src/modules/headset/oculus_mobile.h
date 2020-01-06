#pragma once

extern char *lovrOculusMobileWritablePath;

#ifdef LOVR_ENABLE_OCULUS_AUDIO
void lovrOculusMobileRecreatePose(void* ovrposestatef);
#endif

typedef struct {
  bool live;
  float confidence;
  float handScale;
  BridgeLovrPose pose;
  BridgeLovrPoseList handPoses;
} LovrOculusMobileHands;

LovrOculusMobileHands lovrOculusMobileHands[2];
