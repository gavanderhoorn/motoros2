//ServiceJobPut.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_JOB_PUT_H
#define MOTOROS2_SERVICE_JOB_PUT_H

extern rcl_service_t g_servicePutJob;

typedef struct
{
    motoros2_interfaces__srv__PutJob_Request request;
    motoros2_interfaces__srv__PutJob_Response response;
} ServicePutJob_Messages;
extern ServicePutJob_Messages g_messages_PutJob;

extern void Ros_ServicePutJob_Initialize();
extern void Ros_ServicePutJob_Cleanup();

extern void Ros_ServicePutJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_JOB_PUT_H
