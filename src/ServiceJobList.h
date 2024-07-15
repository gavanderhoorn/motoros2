//ServiceJobList.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_JOB_LIST_H
#define MOTOROS2_SERVICE_JOB_LIST_H

extern rcl_service_t g_serviceListJobs;

typedef struct
{
    motoros2_interfaces__srv__ListJobs_Request request;
    motoros2_interfaces__srv__ListJobs_Response response;
} ServiceListJobs_Messages;
extern ServiceListJobs_Messages g_messages_ListJobs;

extern void Ros_ServiceListJobs_Initialize();
extern void Ros_ServiceListJobs_Cleanup();

extern void Ros_ServiceListJobs_Trigger(const void* request_msg, void* response_msg);

#endif  // MOTOROS2_SERVICE_JOB_LIST_H
