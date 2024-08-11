//ServiceInformJobGet.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_INFORM_JOB_GET_H
#define MOTOROS2_SERVICE_INFORM_JOB_GET_H

extern rcl_service_t g_serviceGetInformJob;

typedef struct
{
    motoros2_interfaces__srv__GetInformJob_Request request;
    motoros2_interfaces__srv__GetInformJob_Response response;
} ServiceGetInformJob_Messages;
extern ServiceGetInformJob_Messages g_messages_GetInformJob;

extern void Ros_ServiceGetInformJob_Initialize();
extern void Ros_ServiceGetInformJob_Cleanup();

extern void Ros_ServiceGetInformJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_INFORM_JOB_GET_H
