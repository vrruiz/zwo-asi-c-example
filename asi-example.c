#include <stdio.h>
#include <stdlib.h>

#include "ASICamera2.h"

int main() {
    printf("ASI ZWO Camera Library Test\n");

    long image_size = 0;
    int image_bytes = 1;

    // Read the number of connected cameras
    int asi_connected_cameras = ASIGetNumOfConnectedCameras();
    printf("Connected cameras: %d\n", asi_connected_cameras);

    if (asi_connected_cameras < 1) return 0;

    // Get each connected camera's properties into an array
    int get_property_success = 0;
    ASI_CAMERA_INFO **asi_camera_info = (ASI_CAMERA_INFO **) malloc(sizeof(ASI_CAMERA_INFO *)*asi_connected_cameras);
    for (int i = 0; i < asi_connected_cameras; i++) {
        asi_camera_info[i] = (ASI_CAMERA_INFO *) malloc(sizeof(ASI_CAMERA_INFO));
        if (ASIGetCameraProperty(asi_camera_info[i], i) == ASI_SUCCESS) {
            get_property_success = 1;
            // Print camera's properties
            printf("Camera #%d\n", i);
            printf("  ASI Camera Name: %s\n", asi_camera_info[i]->Name);
            printf("  Camera ID: %d\n", asi_camera_info[i]->CameraID);
            printf("  Width and Height: %d x %d\n", asi_camera_info[i]->MaxWidth, asi_camera_info[i]->MaxHeight);
            printf("  Color: %s\n", asi_camera_info[i]->IsColorCam == ASI_TRUE ? "Yes" : "No");
            printf("  Bayer pattern: %d\n", asi_camera_info[i]->BayerPattern);
            printf("  Pixel size: %1.1f um\n", asi_camera_info[i]->PixelSize);
            printf("  e-/ADU: %1.2f\n", asi_camera_info[i]->ElecPerADU);
            printf("  Bit depth: %d\n", asi_camera_info[i]->BitDepth);
            printf("  Trigger cam: %s\n", asi_camera_info[i]->IsTriggerCam == 0 ? "No" : "Yes");
        }
    }

    if (get_property_success == 0) return 0;

    int cam = 0; // Camera index

    // Calculate image size
    image_size = asi_camera_info[cam]->MaxWidth * asi_camera_info[cam]->MaxHeight;
    if (asi_camera_info[cam]->IsColorCam == ASI_FALSE) {
        // Mono image
        if (asi_camera_info[cam]->BitDepth > 8) {
            // Bit depth > 8 bits, assume 16 bits
            image_bytes = 2;
        }
    } else {
        // Color camera: assume RGB24 (3 bytes)
        image_bytes = 3;
    }
    image_size *= image_bytes;
    printf("Image size: %d bytes\n", image_size);

    // Open first camera
    printf("Opening camera\n");
    if (ASIOpenCamera(asi_camera_info[cam]->CameraID) != ASI_SUCCESS) {
        printf("Error opening camera #0");
        return 1;
    }

    // Initialize camera
    printf("Initializing camera\n");
    if (ASIInitCamera(asi_camera_info[cam]->CameraID) != ASI_SUCCESS) {
        printf("Error initializing camera #0");
        return 1;
    }

    // Get camera's properties
    int asi_num_controls = 0;
    if (ASIGetNumOfControls(asi_camera_info[cam]->CameraID, &asi_num_controls) != ASI_SUCCESS) {
        printf("Error getting number of controls of camera #0");
        return 1;
    }
    ASI_CONTROL_CAPS **asi_control_caps = (ASI_CONTROL_CAPS **) malloc(sizeof(ASI_CONTROL_CAPS *) * asi_num_controls);
    for (int i = 0; i < asi_num_controls; i++) {
        asi_control_caps[i] = (ASI_CONTROL_CAPS *) malloc(sizeof(ASI_CONTROL_CAPS));
        if (ASIGetControlCaps(asi_camera_info[cam]->CameraID, i, asi_control_caps[i]) == ASI_SUCCESS) {
            // Print camera's properties
            printf("  Property %s: [%d, %d] = %d%s - %s\n",
                asi_control_caps[i]->Name,
                asi_control_caps[i]->MinValue,
                asi_control_caps[i]->MaxValue,
                asi_control_caps[i]->DefaultValue,
                asi_control_caps[i]->IsWritable == 1 ? " (set)" : "",
                asi_control_caps[i]->Description
                );
        }
    }

    // Exposure
    printf("Start exposure...\n");
    ASI_EXPOSURE_STATUS asi_exp_status;
    ASIGetExpStatus(asi_camera_info[cam]->CameraID, &asi_exp_status);
    if (asi_exp_status != ASI_EXP_IDLE) {
        printf("The camera is not idle, cannot start exposure.\n");
        return 1;
    }

    // Set exposure time
    ASI_BOOL b_auto;
    long exposure_time = 20 * 1000000; // 20 seconds
    printf("Set exposure time: %d seconds\n", exposure_time / 1000000);
    if (ASISetControlValue(asi_camera_info[cam]->CameraID, ASI_EXPOSURE, exposure_time, ASI_FALSE) != ASI_SUCCESS) {
        printf("Cannot set exposure time.\n");
        return 1;
    }

    // Start exposure
    if (ASIStartExposure(asi_camera_info[cam]->CameraID, ASI_FALSE) != ASI_SUCCESS) {
        printf("Cannot start exposure.\n");
        return 1;
    }
    int exposing = 1;
    int success = -1;
    // Wait until the exposure has been made
    while (exposing == 1) {
        ASIGetExpStatus(asi_camera_info[cam]->CameraID, &asi_exp_status);
        if (asi_exp_status == ASI_EXP_SUCCESS) {
            printf("Successful exposure\n");
            success = 1;
            exposing = 0;
            break;
        } else if (asi_exp_status == ASI_EXP_FAILED) {
            printf("Failed exposure\n");
            return 1;
        }
    }
    if (success != 1) {
        printf("Cannot happen\n");
        return 1;
    }
    // Get exposure data
    unsigned char *asi_image = (unsigned char *) malloc(sizeof(unsigned char) * image_size);
    if (ASIGetDataAfterExp(asi_camera_info[cam]->CameraID, asi_image, image_size) != ASI_SUCCESS) {
        printf("Couldn't read exposure data\n");
        return 1;
    }
    // Print first bytes of the acquired data
    printf("Image data:\n");
    for (int i = 0; i < 20; i++) {
        printf("%d ", asi_image[i]);
    }
    printf("\n");
    
    // Close camera
    printf("Closing camera\n");
    ASICloseCamera(asi_camera_info[cam]->CameraID);
	
    return 0;
}
