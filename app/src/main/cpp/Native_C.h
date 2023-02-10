#ifndef OBJECTRECOGNITION_NATIVE_C_H
#define OBJECTRECOGNITION_NATIVE_C_H

#include "Util.h"
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>

enum camera_type { BACK_CAMERA, FRONT_CAMERA };

// TODO - reasonable actions with callbacks
// Camera Callbacks
static void CameraDeviceOnDisconnected(void* context, ACameraDevice* device) {
  LOGI("Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}
static void CameraDeviceOnError(void* context, ACameraDevice* device,
                                int error) {
  LOGE("Error(code: %d) on Camera(id: %s).\n", error,
       ACameraDevice_getId(device));
}
// Capture Callbacks
static void CaptureSessionOnReady(void* context,
                                  ACameraCaptureSession* session) {
  LOGI("Session is ready.\n");
}
static void CaptureSessionOnActive(void* context,
                                   ACameraCaptureSession* session) {
  LOGI("Session is activated.\n");
}


class Native_C {
 public:
  explicit Native_C(camera_type type);

  ~Native_C();

  bool MatchCaptureSizeRequest(ImageFormat* resView, int32_t width, int32_t height);

  bool CreateCaptureSession(ANativeWindow* window);

  int32_t GetCameraCount() { return m_camera_id_list->numCameras; }
  uint32_t GetOrientation() { return m_camera_orientation; };

 private:

  // Camera variables
  ACameraDevice* camera_device;
  ACaptureRequest* capture_request;
  ACameraOutputTarget* camera_output_target;
  ACaptureSessionOutput* session_output;
  ACaptureSessionOutputContainer* capture_session_output_container;
  ACameraCaptureSession* capture_session;

  ACameraDevice_StateCallbacks device_state_callbacks;
  ACameraCaptureSession_stateCallbacks capture_session_state_callbacks;

  ACameraManager* camera_manager;
  uint32_t camera_orientation;
  ACameraIdList* camera_id_list = NULL;
  const char* selected_camera_id = NULL;
  bool camera_ready;

  int32_t  backup_width = 480;
  int32_t  backup_height = 720;
};

#endif
