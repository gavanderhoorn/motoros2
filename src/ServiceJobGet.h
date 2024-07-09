//ServiceJobGet.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_JOB_GET_H
#define MOTOROS2_SERVICE_JOB_GET_H

extern rcl_service_t g_serviceGetJob;

typedef struct
{
    motoros2_interfaces__srv__GetJob_Request request;
    motoros2_interfaces__srv__GetJob_Response response;
} ServiceGetJob_Messages;
extern ServiceGetJob_Messages g_messages_GetJob;

extern void Ros_ServiceGetJob_Initialize();
extern void Ros_ServiceGetJob_Cleanup();

extern void Ros_ServiceGetJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_JOB_GET_H
