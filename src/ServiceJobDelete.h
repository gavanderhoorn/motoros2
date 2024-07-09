//ServiceJobDelete.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_JOB_DELETE_H
#define MOTOROS2_SERVICE_JOB_DELETE_H

extern rcl_service_t g_serviceDeleteJob;

typedef struct
{
    motoros2_interfaces__srv__DeleteJob_Request request;
    motoros2_interfaces__srv__DeleteJob_Response response;
} ServiceDeleteJob_Messages;
extern ServiceDeleteJob_Messages g_messages_DeleteJob;

extern void Ros_ServiceDeleteJob_Initialize();
extern void Ros_ServiceDeleteJob_Cleanup();

extern void Ros_ServiceDeleteJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_JOB_DELETE_H
