//ServiceJobGet.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

rcl_service_t g_serviceGetJob;

ServiceGetJob_Messages g_messages_GetJob;

typedef motoros2_interfaces__srv__GetJob_Request GetJobRequest;
typedef motoros2_interfaces__srv__GetJob_Response GetJobResponse;


void Ros_ServiceGetJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_get_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, GetJob);

    rcl_ret_t ret = rclc_service_init_default(&g_serviceGetJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_GET_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK, 
        SUBCODE_FAIL_INIT_SERVICE_GET_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_GetJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_get_job_init);
}

void Ros_ServiceGetJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_get_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_GET_JOB);
    ret = rcl_service_fini(&g_serviceGetJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_GET_JOB " service: %d", ret);
    rosidl_runtime_c__String__fini(&g_messages_GetJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_get_job_fini);
}

void Ros_ServiceGetJob_Trigger(const void* request_msg, void* response_msg)
{
    GetJobRequest const* request = (GetJobRequest*)request_msg;
    GetJobResponse* response = (GetJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to return job with name: '%s'",
        __func__, request->name.data);

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = -1;
    goto DONE;


    //request validation
    if (Ros_strnlen(request->name.data, MAX_JOB_NAME_LEN - 1) == 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Empty job name not allowed");
        response->result_code = -1;
        goto DONE;
    }
    //check against some 'arbitrary' length longer than the allowed maximum
    if (Ros_strnlen(request->name.data, 64) >= MAX_JOB_NAME_LEN)
    {
        rosidl_runtime_c__String__assign(&response->message, "Job name too long");
        response->result_code = -1;
        goto DONE;
    }



DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
