#pragma once

extern char *lovrOculusMobileWritablePath;

typedef struct {
  bool live;
  float confidence;
  float handScale;
  BridgeLovrPose pose;
  BridgeLovrPoseList handPoses;
} LovrOculusMobileHands;

LovrOculusMobileHands lovrOculusMobileHands[2];
