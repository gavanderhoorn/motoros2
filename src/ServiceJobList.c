//ServiceJobList.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

// ?
#define MAX_NUM_JOBS 255

rcl_service_t g_serviceListJobs;

ServiceListJobs_Messages g_messages_ListJobs;

typedef motoros2_interfaces__srv__ListJobs_Response ListJobsResponse;


void Ros_ServiceListJobs_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_list_jobs_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, ListJobs);

    rcl_ret_t ret = rclc_service_init_default(&g_serviceListJobs,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_LIST_JOBS);
    motoRosAssert_withMsg(ret == RCL_RET_OK, 
        SUBCODE_FAIL_INIT_SERVICE_LIST_JOBS, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_ListJobs.response.message);
    rosidl_runtime_c__String__Sequence__init(
        &g_messages_ListJobs.response.names, MAX_NUM_JOBS);
    g_messages_ListJobs.response.names.size = 0;

    MOTOROS2_MEM_TRACE_REPORT(svc_list_jobs_init);
}

void Ros_ServiceListJobs_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_list_jobs_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_LIST_JOBS);
    ret = rcl_service_fini(&g_serviceListJobs, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_LIST_JOBS " service: %d", ret);
    rosidl_runtime_c__String__fini(&g_messages_ListJobs.response.message);
    rosidl_runtime_c__String__Sequence__fini(&g_messages_ListJobs.response.names);

    MOTOROS2_MEM_TRACE_REPORT(svc_list_jobs_fini);
}

void Ros_ServiceListJobs_Trigger(const void* request_msg, void* response_msg)
{
    RCL_UNUSED(request_msg);
    ListJobsResponse* response = (ListJobsResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    //TODO(gavanderhoorn): retrieve names of existing jobs, populate response
    //TODO(gavanderhoorn): should we init response.names here, based on nr of jobs?

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = -1;

    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
