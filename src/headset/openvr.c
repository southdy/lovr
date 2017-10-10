#include "headset/openvr.h"
#include "event/event.h"
#include "graphics/graphics.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static HeadsetState state;

static ControllerButton getButton(uint32_t button, ControllerHand hand) {
  switch (state.type) {
    case HEADSET_RIFT:
      switch (button) {
        case EVRButtonId_k_EButton_Axis1: return CONTROLLER_BUTTON_TRIGGER;
        case EVRButtonId_k_EButton_Axis2: return CONTROLLER_BUTTON_GRIP;
        case EVRButtonId_k_EButton_Axis0: return CONTROLLER_BUTTON_TOUCHPAD;
        case EVRButtonId_k_EButton_A:
          switch (hand) {
            case HAND_LEFT: return CONTROLLER_BUTTON_X;
            case HAND_RIGHT: return CONTROLLER_BUTTON_A;
            default: return CONTROLLER_BUTTON_UNKNOWN;
          }
        case EVRButtonId_k_EButton_ApplicationMenu:
          switch (hand) {
            case HAND_LEFT: return CONTROLLER_BUTTON_Y;
            case HAND_RIGHT: return CONTROLLER_BUTTON_B;
            default: return CONTROLLER_BUTTON_UNKNOWN;
          }
        default: return CONTROLLER_BUTTON_UNKNOWN;
      }
      break;

    default:
      switch (button) {
        case EVRButtonId_k_EButton_System: return CONTROLLER_BUTTON_SYSTEM;
        case EVRButtonId_k_EButton_ApplicationMenu: return CONTROLLER_BUTTON_MENU;
        case EVRButtonId_k_EButton_SteamVR_Trigger: return CONTROLLER_BUTTON_TRIGGER;
        case EVRButtonId_k_EButton_Grip: return CONTROLLER_BUTTON_GRIP;
        case EVRButtonId_k_EButton_SteamVR_Touchpad: return CONTROLLER_BUTTON_TOUCHPAD;
        default: return CONTROLLER_BUTTON_UNKNOWN;
      }
  }

  return CONTROLLER_BUTTON_UNKNOWN;
}

static int getButtonState(uint64_t mask, ControllerButton button, ControllerHand hand) {
  switch (state.type) {
    case HEADSET_RIFT:
      switch (button) {
        case CONTROLLER_BUTTON_TRIGGER: return (mask >> EVRButtonId_k_EButton_Axis1) & 1;
        case CONTROLLER_BUTTON_GRIP: return (mask >> EVRButtonId_k_EButton_Axis2) & 1;
        case CONTROLLER_BUTTON_TOUCHPAD: return (mask >> EVRButtonId_k_EButton_Axis0) & 1;
        case CONTROLLER_BUTTON_A: return hand == HAND_RIGHT && (mask >> EVRButtonId_k_EButton_A) & 1;
        case CONTROLLER_BUTTON_B: return hand == HAND_RIGHT && (mask >> EVRButtonId_k_EButton_ApplicationMenu) & 1;
        case CONTROLLER_BUTTON_X: return hand == HAND_LEFT && (mask >> EVRButtonId_k_EButton_A) & 1;
        case CONTROLLER_BUTTON_Y: return hand == HAND_LEFT && (mask >> EVRButtonId_k_EButton_ApplicationMenu) & 1;
        default: return 0;
      }

    default:
      switch (button) {
        case CONTROLLER_BUTTON_SYSTEM: return (mask >> EVRButtonId_k_EButton_System) & 1;
        case CONTROLLER_BUTTON_MENU: return (mask >> EVRButtonId_k_EButton_ApplicationMenu) & 1;
        case CONTROLLER_BUTTON_TRIGGER: return (mask >> EVRButtonId_k_EButton_SteamVR_Trigger) & 1;
        case CONTROLLER_BUTTON_GRIP: return (mask >> EVRButtonId_k_EButton_Grip) & 1;
        case CONTROLLER_BUTTON_TOUCHPAD: return (mask >> EVRButtonId_k_EButton_SteamVR_Touchpad) & 1;
        default: return 0;
      }
  }
}

static TrackedDevicePose_t getPose(unsigned int deviceIndex) {
  if (state.isRendering) {
    return state.renderPoses[deviceIndex];
  }

  ETrackingUniverseOrigin origin = ETrackingUniverseOrigin_TrackingUniverseStanding;
  float timeSinceVsync;
  state.system->GetTimeSinceLastVsync(&timeSinceVsync, NULL);
  float frameDuration = 1.f / state.refreshRate;
  float secondsInFuture = frameDuration - timeSinceVsync + state.vsyncToPhotons;
  TrackedDevicePose_t poses[16];
  state.system->GetDeviceToAbsoluteTrackingPose(origin, secondsInFuture, poses, 16);
  return poses[deviceIndex];
}

void lovrHeadsetInit() {
  state.isInitialized = 0;
  state.isRendering = 0;
  state.isMirrored = 1;
  state.texture = NULL;
  vec_init(&state.controllers);

  for (int i = 0; i < 16; i++) {
    state.deviceModels[i] = NULL;
    state.deviceTextures[i] = NULL;
  }

  if (!VR_IsHmdPresent() || !VR_IsRuntimeInstalled()) {
    lovrHeadsetDestroy();
    return;
  }

  EVRInitError vrError;
  VR_InitInternal(&vrError, EVRApplicationType_VRApplication_Scene);

  if (vrError != EVRInitError_VRInitError_None) {
    lovrHeadsetDestroy();
    return;
  }

  char buffer[128];

  sprintf(buffer, "FnTable:%s", IVRSystem_Version);
  state.system = (struct VR_IVRSystem_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  if (vrError != EVRInitError_VRInitError_None || !state.system) {
    lovrHeadsetDestroy();
    return;
  }

  sprintf(buffer, "FnTable:%s", IVRCompositor_Version);
  state.compositor = (struct VR_IVRCompositor_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  if (vrError != EVRInitError_VRInitError_None || !state.compositor) {
    lovrHeadsetDestroy();
    return;
  }

  sprintf(buffer, "FnTable:%s", IVRChaperone_Version);
  state.chaperone = (struct VR_IVRChaperone_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  if (vrError != EVRInitError_VRInitError_None || !state.chaperone) {
    lovrHeadsetDestroy();
    return;
  }

  sprintf(buffer, "FnTable:%s", IVRRenderModels_Version);
  state.renderModels = (struct VR_IVRRenderModels_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  if (vrError != EVRInitError_VRInitError_None || !state.renderModels) {
    lovrHeadsetDestroy();
    return;
  }

  state.isInitialized = 1;
  state.headsetIndex = k_unTrackedDeviceIndex_Hmd;

  state.system->GetStringTrackedDeviceProperty(state.headsetIndex, ETrackedDeviceProperty_Prop_ManufacturerName_String, buffer, 128, NULL);
  if (!strncmp(buffer, "HTC", 128)) {
    state.type = HEADSET_VIVE;
  } else if (!strncmp(buffer, "Oculus", 128)) {
    state.type = HEADSET_RIFT;
  } else {
    state.type = HEADSET_UNKNOWN;
  }

  state.refreshRate = state.system->GetFloatTrackedDeviceProperty(state.headsetIndex, ETrackedDeviceProperty_Prop_DisplayFrequency_Float, NULL);
  state.vsyncToPhotons = state.system->GetFloatTrackedDeviceProperty(state.headsetIndex, ETrackedDeviceProperty_Prop_SecondsFromVsyncToPhotons_Float, NULL);

  state.clipNear = 0.1f;
  state.clipFar = 30.f;
  lovrHeadsetRefreshControllers();
  lovrEventAddPump(lovrHeadsetPoll);
  atexit(lovrHeadsetDestroy);
}

void lovrHeadsetDestroy() {
  state.isInitialized = 0;
  if (state.texture) {
    lovrRelease(&state.texture->ref);
  }
  for (int i = 0; i < 16; i++) {
    if (state.deviceModels[i]) {
      state.renderModels->FreeRenderModel(state.deviceModels[i]);
    }
    state.deviceModels[i] = NULL;
    state.deviceTextures[i] = NULL;
  }
  Controller* controller; int i;
  vec_foreach(&state.controllers, controller, i) {
    lovrRelease(&controller->ref);
  }
  vec_deinit(&state.controllers);
}

void lovrHeadsetPoll() {
  if (!state.isInitialized) return;
  struct VREvent_t vrEvent;
  while (state.system->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
    switch (vrEvent.eventType) {
      case EVREventType_VREvent_TrackedDeviceActivated:
      case EVREventType_VREvent_TrackedDeviceDeactivated:
      case EVREventType_VREvent_TrackedDeviceRoleChanged: {
        lovrHeadsetRefreshControllers();
        break;
      }

      case EVREventType_VREvent_ButtonPress:
      case EVREventType_VREvent_ButtonUnpress: {
        int isPress = vrEvent.eventType == EVREventType_VREvent_ButtonPress;
        Controller* controller;
        int i;
        vec_foreach(&state.controllers, controller, i) {
          if (controller->id == vrEvent.trackedDeviceIndex) {
            ControllerHand hand = lovrHeadsetControllerGetHand(controller);
            Event event;
            if (isPress) {
              event.type = EVENT_CONTROLLER_PRESSED;
              event.data.controllerpressed.controller = controller;
              event.data.controllerpressed.button = getButton(vrEvent.data.controller.button, hand);
            } else {
              event.type = EVENT_CONTROLLER_RELEASED;
              event.data.controllerreleased.controller = controller;
              event.data.controllerreleased.button = getButton(vrEvent.data.controller.button, hand);
            }
            lovrEventPush(event);
            break;
          }
        }
        break;
      }

      case EVREventType_VREvent_InputFocusCaptured:
      case EVREventType_VREvent_InputFocusReleased: {
        int isFocused = vrEvent.eventType == EVREventType_VREvent_InputFocusReleased;
        EventData data = { .focus = { isFocused } };
        Event event = { .type = EVENT_FOCUS, .data = data };
        lovrEventPush(event);
        break;
      }
    }
  }
}

int lovrHeadsetIsPresent() {
  return state.isInitialized && state.system->IsTrackedDeviceConnected(state.headsetIndex);
}

HeadsetType lovrHeadsetGetType() {
  return state.type;
}

HeadsetOrigin lovrHeadsetGetOriginType() {
  if (!state.isInitialized) {
    return ORIGIN_HEAD;
  }

  switch (state.compositor->GetTrackingSpace()) {
    case ETrackingUniverseOrigin_TrackingUniverseSeated: return ORIGIN_HEAD;
    case ETrackingUniverseOrigin_TrackingUniverseStanding: return ORIGIN_FLOOR;
    default: return ORIGIN_HEAD;
  }
}

int lovrHeadsetIsMirrored() {
  return state.isMirrored;
}

void lovrHeadsetSetMirrored(int mirror) {
  state.isMirrored = mirror;
}

void lovrHeadsetGetDisplayDimensions(int* width, int* height) {
  if (!state.isInitialized) {
    *width = *height = 0;
  } else {
    *width = state.renderWidth;
    *height = state.renderHeight;
  }
}

void lovrHeadsetGetClipDistance(float* near, float* far) {
  if (!state.isInitialized) {
    *near = *far = 0.f;
  } else {
    *near = state.clipNear;
    *far = state.clipFar;
  }
}

void lovrHeadsetSetClipDistance(float near, float far) {
  if (!state.isInitialized) return;
  state.clipNear = near;
  state.clipFar = far;
}

float lovrHeadsetGetBoundsWidth() {
  if (!state.isInitialized) return 0.f;
  float width;
  state.chaperone->GetPlayAreaSize(&width, NULL);
  return width;
}

float lovrHeadsetGetBoundsDepth() {
  if (!state.isInitialized) return 0.f;
  float depth;
  state.chaperone->GetPlayAreaSize(NULL, &depth);
  return depth;
}

void lovrHeadsetGetBoundsGeometry(float* geometry) {
  if (!state.isInitialized) {
    memset(geometry, 0, 12 * sizeof(float));
  } else {
    struct HmdQuad_t quad;
    state.chaperone->GetPlayAreaRect(&quad);
    for (int i = 0; i < 4; i++) {
      geometry[3 * i + 0] = quad.vCorners[i].v[0];
      geometry[3 * i + 1] = quad.vCorners[i].v[1];
      geometry[3 * i + 2] = quad.vCorners[i].v[2];
    }
  }
}

void lovrHeadsetGetPosition(float* x, float* y, float* z) {
  if (!state.isInitialized) {
    *x = *y = *z = 0.f;
    return;
  }

  TrackedDevicePose_t pose = getPose(state.headsetIndex);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *x = *y = *z = 0.f;
    return;
  }

  *x = pose.mDeviceToAbsoluteTracking.m[0][3];
  *y = pose.mDeviceToAbsoluteTracking.m[1][3];
  *z = pose.mDeviceToAbsoluteTracking.m[2][3];
}

void lovrHeadsetGetEyePosition(HeadsetEye eye, float* x, float* y, float* z) {
  if (!state.isInitialized) {
    *x = *y = *z = 0.f;
    return;
  }

  TrackedDevicePose_t pose = getPose(state.headsetIndex);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *x = *y = *z = 0.f;
    return;
  }

  float transform[16];
  float eyeTransform[16];
  EVREye vrEye = (eye == EYE_LEFT) ? EVREye_Eye_Left : EVREye_Eye_Right;
  mat4_fromMat34(eyeTransform, state.system->GetEyeToHeadTransform(vrEye).m);
  mat4_fromMat44(transform, pose.mDeviceToAbsoluteTracking.m);
  mat4_multiply(transform, eyeTransform);

  *x = transform[12];
  *y = transform[13];
  *z = transform[14];
}

void lovrHeadsetGetOrientation(float* angle, float* x, float* y, float *z) {
  if (!state.isInitialized) {
    *angle = *x = *y = *z = 0.f;
    return;
  }

  TrackedDevicePose_t pose = getPose(state.headsetIndex);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *angle = *x = *y = *z = 0.f;
    return;
  }

  float matrix[16];
  float rotation[4];
  quat_fromMat4(rotation, mat4_fromMat44(matrix, pose.mDeviceToAbsoluteTracking.m));
  quat_getAngleAxis(rotation, angle, x, y, z);
}

void lovrHeadsetGetVelocity(float* x, float* y, float* z) {
  if (!state.isInitialized) {
    *x = *y = *z = 0.f;
    return;
  }

  TrackedDevicePose_t pose = getPose(state.headsetIndex);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *x = *y = *z = 0.f;
    return;
  }

  *x = pose.vVelocity.v[0];
  *y = pose.vVelocity.v[1];
  *z = pose.vVelocity.v[2];
}

void lovrHeadsetGetAngularVelocity(float* x, float* y, float* z) {
  if (!state.isInitialized) {
    *x = *y = *z = 0.f;
    return;
  }

  TrackedDevicePose_t pose = getPose(state.headsetIndex);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *x = *y = *z = 0.f;
    return;
  }

  *x = pose.vAngularVelocity.v[0];
  *y = pose.vAngularVelocity.v[1];
  *z = pose.vAngularVelocity.v[2];
}

void lovrHeadsetRefreshControllers() {
  if (!state.isInitialized) return;

  unsigned int leftHand = ETrackedControllerRole_TrackedControllerRole_LeftHand;
  unsigned int leftControllerId = state.system->GetTrackedDeviceIndexForControllerRole(leftHand);

  unsigned int rightHand = ETrackedControllerRole_TrackedControllerRole_RightHand;
  unsigned int rightControllerId = state.system->GetTrackedDeviceIndexForControllerRole(rightHand);

  unsigned int controllerIds[2] = { leftControllerId, rightControllerId };

  // Remove controllers that are no longer recognized as connected
  Controller* controller; int i;
  vec_foreach_rev(&state.controllers, controller, i) {
    if (controller->id != controllerIds[0] && controller->id != controllerIds[1]) {
      EventType type = EVENT_CONTROLLER_REMOVED;
      EventData data = { .controllerremoved = { controller } };
      Event event = { .type = type, .data = data };
      lovrRetain(&controller->ref);
      lovrEventPush(event);
      vec_splice(&state.controllers, i, 1);
      lovrRelease(&controller->ref);
    }
  }

  // Add connected controllers that aren't in the list yet
  for (i = 0; i < 2; i++) {
    if ((int) controllerIds[i] != -1) {
      controller = lovrHeadsetAddController(controllerIds[i]);
      if (!controller) continue;
      EventType type = EVENT_CONTROLLER_ADDED;
      EventData data = { .controlleradded = { controller } };
      Event event = { .type = type, .data = data };
      lovrRetain(&controller->ref);
      lovrEventPush(event);
    }
  }
}

Controller* lovrHeadsetAddController(unsigned int deviceIndex) {
  if (!state.isInitialized) return NULL;

  if ((int) deviceIndex == -1) {
    return NULL;
  }

  Controller* controller; int i;
  vec_foreach(&state.controllers, controller, i) {
    if (controller->id == deviceIndex) {
      return NULL;
    }
  }

  controller = lovrAlloc(sizeof(Controller), lovrControllerDestroy);
  controller->id = deviceIndex;
  vec_push(&state.controllers, controller);
  return controller;
}

vec_controller_t* lovrHeadsetGetControllers() {
  if (!state.isInitialized) return NULL;
  return &state.controllers;
}

int lovrHeadsetControllerIsPresent(Controller* controller) {
  if (!state.isInitialized || !controller) return 0;
  return state.system->IsTrackedDeviceConnected(controller->id);
}

ControllerHand lovrHeadsetControllerGetHand(Controller* controller) {
  if (!state.isInitialized || !controller) return HAND_UNKNOWN;
  switch (state.system->GetControllerRoleForTrackedDeviceIndex(controller->id)) {
    case ETrackedControllerRole_TrackedControllerRole_LeftHand: return HAND_LEFT;
    case ETrackedControllerRole_TrackedControllerRole_RightHand: return HAND_RIGHT;
    default: return HAND_UNKNOWN;
  }
}

void lovrHeadsetControllerGetPosition(Controller* controller, float* x, float* y, float* z) {
  if (!state.isInitialized || !controller) {
    *x = *y = *z = 0.f;
  }

  TrackedDevicePose_t pose = getPose(controller->id);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *x = *y = *z = 0.f;
    return;
  }

  *x = pose.mDeviceToAbsoluteTracking.m[0][3];
  *y = pose.mDeviceToAbsoluteTracking.m[1][3];
  *z = pose.mDeviceToAbsoluteTracking.m[2][3];
}

void lovrHeadsetControllerGetOrientation(Controller* controller, float* angle, float* x, float* y, float* z) {
  if (!state.isInitialized || !controller) {
    *angle = *x = *y = *z = 0.f;
  }

  TrackedDevicePose_t pose = getPose(controller->id);

  if (!pose.bPoseIsValid || !pose.bDeviceIsConnected) {
    *angle = *x = *y = *z = 0.f;
    return;
  }

  float matrix[16];
  float rotation[4];
  quat_fromMat4(rotation, mat4_fromMat44(matrix, pose.mDeviceToAbsoluteTracking.m));
  quat_getAngleAxis(rotation, angle, x, y, z);
}

float lovrHeadsetControllerGetAxis(Controller* controller, ControllerAxis axis) {
  if (!state.isInitialized || !controller) return 0.f;

  VRControllerState_t input;
  state.system->GetControllerState(controller->id, &input, sizeof(input));

  switch (state.type) {
    case HEADSET_RIFT:
      switch (axis) {
        case CONTROLLER_AXIS_TRIGGER: return input.rAxis[1].x;
        case CONTROLLER_AXIS_GRIP: return input.rAxis[2].x;
        case CONTROLLER_AXIS_TOUCHPAD_X: return input.rAxis[0].x;
        case CONTROLLER_AXIS_TOUCHPAD_Y: return input.rAxis[0].y;
        default: return 0;
      }

    default:
      switch (axis) {
        case CONTROLLER_AXIS_TRIGGER: return input.rAxis[1].x;
        case CONTROLLER_AXIS_TOUCHPAD_X: return input.rAxis[0].x;
        case CONTROLLER_AXIS_TOUCHPAD_Y: return input.rAxis[0].y;
        default: return 0;
      }
  }

  return 0;
}

int lovrHeadsetControllerIsDown(Controller* controller, ControllerButton button) {
  if (!state.isInitialized || !controller) return 0;

  VRControllerState_t input;
  state.system->GetControllerState(controller->id, &input, sizeof(input));
  ControllerHand hand = lovrHeadsetControllerGetHand(controller);
  return getButtonState(input.ulButtonPressed, button, hand);
}

int lovrHeadsetControllerIsTouched(Controller* controller, ControllerButton button) {
  if (!state.isInitialized || !controller) return 0;

  VRControllerState_t input;
  state.system->GetControllerState(controller->id, &input, sizeof(input));
  ControllerHand hand = lovrHeadsetControllerGetHand(controller);
  return getButtonState(input.ulButtonTouched, button, hand);
}

void lovrHeadsetControllerVibrate(Controller* controller, float duration, float power) {
  if (!state.isInitialized || !controller || duration <= 0) return;

  uint32_t axis = 0;
  unsigned short uSeconds = (unsigned short) (duration * 1e6);
  state.system->TriggerHapticPulse(controller->id, axis, uSeconds);
}

ModelData* lovrHeadsetControllerNewModelData(Controller* controller) {
  if (!state.isInitialized || !controller) return NULL;

  int id = controller->id;

  // Get model name
  char renderModelName[1024];
  ETrackedDeviceProperty renderModelNameProperty = ETrackedDeviceProperty_Prop_RenderModelName_String;
  state.system->GetStringTrackedDeviceProperty(controller->id, renderModelNameProperty, renderModelName, 1024, NULL);

  // Load model
  if (!state.deviceModels[id]) {
    while (state.renderModels->LoadRenderModel_Async(renderModelName, &state.deviceModels[id]) == EVRRenderModelError_VRRenderModelError_Loading) {
      lovrSleep(.001);
    }
  }

  RenderModel_t* vrModel = state.deviceModels[id];

  ModelData* modelData = malloc(sizeof(ModelData));
  if (!modelData) return NULL;

  modelData->indexCount = vrModel->unTriangleCount;
  modelData->indices = malloc(modelData->indexCount * sizeof(unsigned int));
  memcpy(modelData->indices, vrModel->rIndexData, modelData->indexCount * sizeof(unsigned int));

  modelData->vertexCount = vrModel->unVertexCount;
  modelData->vertexSize = 8;
  modelData->vertices = malloc(modelData->vertexCount * modelData->vertexSize * sizeof(float));

  int vertex = 0;
  for (size_t i = 0; i < vrModel->unVertexCount; i++) {
    float* position = vrModel->rVertexData[i].vPosition.v;
    float* normal = vrModel->rVertexData[i].vNormal.v;
    float* texCoords = vrModel->rVertexData[i].rfTextureCoord;

    modelData->vertices[vertex++] = position[0];
    modelData->vertices[vertex++] = position[1];
    modelData->vertices[vertex++] = position[2];

    modelData->vertices[vertex++] = normal[0];
    modelData->vertices[vertex++] = normal[1];
    modelData->vertices[vertex++] = normal[2];

    modelData->vertices[vertex++] = texCoords[0];
    modelData->vertices[vertex++] = texCoords[1];
  }

  modelData->nodeCount = 1;
  modelData->primitiveCount = 1;

  modelData->nodes = malloc(sizeof(ModelNode));
  modelData->primitives = malloc(sizeof(ModelPrimitive));

  ModelNode* root = &modelData->nodes[0];
  root->parent = -1;
  mat4_identity(root->transform);
  vec_init(&root->children);
  vec_init(&root->primitives);
  vec_push(&root->primitives, 0);
  modelData->primitives[0].drawStart = 0;
  modelData->primitives[0].drawCount = modelData->vertexCount;

  modelData->hasNormals = 1;
  modelData->hasUVs = 1;

  return modelData;
}

TextureData* lovrHeadsetControllerNewTextureData(Controller* controller) {
  if (!state.isInitialized || !controller) return NULL;

  int id = controller->id;

  if (!state.deviceModels[id]) {
    lovrHeadsetControllerNewModelData(controller);
  }

  if (!state.deviceTextures[id]) {
    while (state.renderModels->LoadTexture_Async(state.deviceModels[id]->diffuseTextureId, &state.deviceTextures[id]) == EVRRenderModelError_VRRenderModelError_Loading) {
      lovrSleep(.001);
    }
  }

  RenderModel_TextureMap_t* vrTexture = state.deviceTextures[id];

  TextureData* textureData = malloc(sizeof(TextureData));
  if (!textureData) return NULL;

  int width = vrTexture->unWidth;
  int height = vrTexture->unHeight;
  TextureFormat format = FORMAT_RGBA;
  size_t size = width * height * format.blockBytes;

  textureData->width = width;
  textureData->height = height;
  textureData->format = format;
  textureData->data = memcpy(malloc(size), vrTexture->rubTextureMapData, size);;
  textureData->mipmaps.generated = 1;
  textureData->blob = NULL;
  return textureData;
}

void lovrHeadsetRenderTo(headsetRenderCallback callback, void* userdata) {
  if (!state.isInitialized) return;

  if (!state.texture) {
    state.system->GetRecommendedRenderTargetSize(&state.renderWidth, &state.renderHeight);
    TextureData* textureData = lovrTextureDataGetEmpty(state.renderWidth, state.renderHeight, FORMAT_RGBA);
    state.texture = lovrTextureCreateWithFramebuffer(textureData, PROJECTION_PERSPECTIVE, 4);
  }

  float head[16], transform[16], projection[16];
  float (*matrix)[4];

  lovrGraphicsPushCanvas();
  state.isRendering = 1;
  state.compositor->WaitGetPoses(state.renderPoses, 16, NULL, 0);

  // Head transform
  matrix = state.renderPoses[state.headsetIndex].mDeviceToAbsoluteTracking.m;
  mat4_invert(mat4_fromMat34(head, matrix));

  for (HeadsetEye eye = EYE_LEFT; eye <= EYE_RIGHT; eye++) {

    // Eye transform
    EVREye vrEye = (eye == EYE_LEFT) ? EVREye_Eye_Left : EVREye_Eye_Right;
    matrix = state.system->GetEyeToHeadTransform(vrEye).m;
    mat4_invert(mat4_fromMat34(transform, matrix));
    mat4_multiply(transform, head);

    // Projection
    matrix = state.system->GetProjectionMatrix(vrEye, state.clipNear, state.clipFar).m;
    mat4_fromMat44(projection, matrix);

    // Render
    lovrTextureBindFramebuffer(state.texture);
    lovrGraphicsPush();
    lovrGraphicsMatrixTransform(MATRIX_VIEW, transform);
    lovrGraphicsSetProjection(projection);
    lovrGraphicsClear(1, 1);
    callback(eye, userdata);
    lovrGraphicsPop();
    lovrTextureResolveMSAA(state.texture);

    // OpenVR changes the OpenGL texture binding, so we reset it after rendering
    //Texture* oldTexture = lovrGraphicsGetTexture();

    // Submit
    uintptr_t texture = (uintptr_t) state.texture->id;
    ETextureType textureType = ETextureType_TextureType_OpenGL;
    Texture_t eyeTexture = { (void*) texture, textureType, EColorSpace_ColorSpace_Gamma };
    EVRSubmitFlags flags = EVRSubmitFlags_Submit_Default;
    state.compositor->Submit(vrEye, &eyeTexture, NULL, flags);

    // Reset to the correct texture
    //glBindTexture(GL_TEXTURE_2D, oldTexture->id);
  }

  state.isRendering = 0;
  lovrGraphicsPopCanvas();

  if (state.isMirrored) {
    Color oldColor = lovrGraphicsGetColor();
    lovrGraphicsSetColor((Color) { 255, 255, 255, 255 });
    //Shader* lastShader = lovrGraphicsGetShader();
    //lovrGraphicsSetShader(NULL);
    lovrGraphicsPlaneFullscreen(state.texture);
    //lovrGraphicsSetShader(lastShader);
    lovrGraphicsSetColor(oldColor);
  }
}
