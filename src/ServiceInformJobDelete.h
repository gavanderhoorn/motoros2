//ServiceInformJobDelete.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_INFORM_JOB_DELETE_H
#define MOTOROS2_SERVICE_INFORM_JOB_DELETE_H

extern rcl_service_t g_serviceDeleteInformJob;

typedef struct
{
    motoros2_interfaces__srv__DeleteInformJob_Request request;
    motoros2_interfaces__srv__DeleteInformJob_Response response;
} ServiceDeleteInformJob_Messages;
extern ServiceDeleteInformJob_Messages g_messages_DeleteInformJob;

extern void Ros_ServiceDeleteInformJob_Initialize();
extern void Ros_ServiceDeleteInformJob_Cleanup();

extern void Ros_ServiceDeleteInformJob_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_INFORM_JOB_DELETE_H
