//ServiceInformJobPut.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_INFORM_JOB_PUT_H
#define MOTOROS2_SERVICE_INFORM_JOB_PUT_H

extern rcl_service_t g_servicePutInformJob;

typedef struct
{
    motoros2_interfaces__srv__PutInformJob_Request request;
    motoros2_interfaces__srv__PutInformJob_Response response;
} ServicePutInformJob_Messages;
extern ServicePutInformJob_Messages g_messages_PutInformJob;

extern void Ros_ServicePutInformJob_Initialize();
extern void Ros_ServicePutInformJob_Cleanup();

extern void Ros_ServicePutInformJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_INFORM_JOB_PUT_H
