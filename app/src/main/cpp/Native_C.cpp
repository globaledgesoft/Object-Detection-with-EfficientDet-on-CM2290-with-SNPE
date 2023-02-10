#include "Native_C.h"

Native_C::Native_C(camera_type type) {

  ACameraMetadata* cameraMetadata = nullptr;
  camera_status_t cameraStatus = ACAMERA_OK;

  camera_manager = ACameraManager_create();

  cameraStatus =
      ACameraManager_getCameraIdList(camera_manager, &camera_id_list);
  ASSERT(cameraStatus == ACAMERA_OK,
         "Failed to get camera id list (reason: %d)", cameraStatus);
  ASSERT(camera_id_list->numCameras > 0, "No camera device detected");


  if (type == BACK_CAMERA) {
    selected_camera_id = camera_id_list->cameraIds[0];
    camera_orientation = 0;
  } else {
    ASSERT(camera_id_list->numCameras > 1, "No dual camera setup");
    selected_camera_id= camera_id_list->cameraIds[1];
    camera_orientation = 270;
  }
  cameraStatus = ACameraManager_getCameraCharacteristics(
      camera_manager, selected_camera_id, &cameraMetadata);
  ASSERT(cameraStatus == ACAMERA_OK, "Failed to get camera meta data of ID: %s",
         selected_camera_id);

  device_state_callbacks.onDisconnected = CameraDeviceOnDisconnected;
  device_state_callbacks.onError = CameraDeviceOnError;

  cameraStatus =
      ACameraManager_openCamera(camera_manager, selected_camera_id,
                                &device_state_callbacks, &camera_device);
  ASSERT(cameraStatus == ACAMERA_OK, "Failed to open camera device (id: %s)",
         selected_camera_id);



  camera_ready = true;
}

Native_C::~Native_C() {
  if (capture_request != nullptr) {
    ACaptureRequest_free(capture_request);
    capture_request = nullptr;
  }

  if (camera_output_target != nullptr) {
    ACameraOutputTarget_free(camera_output_target);
    camera_output_target = nullptr;
  }

  if (camera_device != nullptr) {
    ACameraDevice_close(camera_device);
    camera_device = nullptr;
  }

  ACaptureSessionOutputContainer_remove(capture_session_output_container,
                                        session_output);
  if (session_output != nullptr) {
    ACaptureSessionOutput_free(session_output);
    session_output = nullptr;
  }

  if (capture_session_output_container != nullptr) {
    ACaptureSessionOutputContainer_free(capture_session_output_container);
    capture_session_output_container = nullptr;
  }

  ACameraManager_delete(camera_manager);
}

bool Native_C::MatchCaptureSizeRequest(ImageFormat* resView, int32_t width, int32_t height) {
  Display_Dimension disp(width, height);
  if (camera_orientation == 90 || camera_orientation == 270) {
    disp.Flip();
  }

  ACameraMetadata* metadata;
  ACameraManager_getCameraCharacteristics(camera_manager,
                                          selected_camera_id, &metadata);
  ACameraMetadata_const_entry entry;
  ACameraMetadata_getConstEntry(
      metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
  // format of the data: format, width, height, input?, type int32
  bool foundIt = false;
  Display_Dimension foundRes(1000, 1000); // max resolution for current gen phones

  for (int i = 0; i < entry.count; ++i) {
    int32_t input = entry.data.i32[i * 4 + 3];
    int32_t format = entry.data.i32[i * 4 + 0];
    if (input) continue;

    if (format == AIMAGE_FORMAT_YUV_420_888 || format == AIMAGE_FORMAT_JPEG) {
      Display_Dimension res(entry.data.i32[i * 4 + 1],
                           entry.data.i32[i * 4 + 2]);
      if (!disp.IsSameRatio(res)) continue;
      if (format == AIMAGE_FORMAT_YUV_420_888 && foundRes > res) {
        foundIt = true;
        foundRes = res;
      }
    }
  }

  if (foundIt) {
    resView->width = foundRes.org_width();
    resView->height = foundRes.org_height();
  } else {
    if (disp.IsPortrait()) {
      resView->width = 1080;
      resView->height = 1920;
    } else {
      resView->width = 1920;
      resView->height = 1080;
    }
  }
  resView->format = AIMAGE_FORMAT_YUV_420_888;
  LOGI("--- W -- H -- %d -- %d",resView->width,resView->height);
  return foundIt;
}

bool Native_C::CreateCaptureSession(ANativeWindow* window) {

  camera_status_t cameraStatus = ACAMERA_OK;

  ACaptureSessionOutputContainer_create(&capture_session_output_container);
  ANativeWindow_acquire(window);
  ACaptureSessionOutput_create(window, &session_output);
  ACaptureSessionOutputContainer_add(capture_session_output_container,
                                     session_output);
  ACameraOutputTarget_create(window, &camera_output_target);


  cameraStatus = ACameraDevice_createCaptureRequest(camera_device,
                                                    TEMPLATE_RECORD, &capture_request);
  ASSERT(cameraStatus == ACAMERA_OK,
         "Failed to create preview capture request (id: %s)",
         selected_camera_id);

  ACaptureRequest_addTarget(capture_request, camera_output_target);

  capture_session_state_callbacks.onReady = CaptureSessionOnReady;
  capture_session_state_callbacks.onActive = CaptureSessionOnActive;
  ACameraDevice_createCaptureSession(
      camera_device, capture_session_output_container,
      &capture_session_state_callbacks, &capture_session);

  ACameraCaptureSession_setRepeatingRequest(capture_session, nullptr, 1,
                                            &capture_request, nullptr);

  return true;
}