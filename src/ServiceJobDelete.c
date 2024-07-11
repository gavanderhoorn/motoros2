//ServiceJobDelete.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

#define RAW_CHAR_P(micro_ros_str) (micro_ros_str.data)

rcl_service_t g_serviceDeleteJob;

ServiceDeleteJob_Messages g_messages_DeleteJob;

typedef motoros2_interfaces__srv__DeleteJob_Request DeleteJobRequest;
typedef motoros2_interfaces__srv__DeleteJob_Response DeleteJobResponse;


void Ros_ServiceDeleteJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_delete_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, DeleteJob);

    rcl_ret_t ret = rclc_service_init_default(&g_serviceDeleteJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_DELETE_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK, 
        SUBCODE_FAIL_INIT_SERVICE_DELETE_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_DeleteJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_delete_job_init);
}

void Ros_ServiceDeleteJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_delete_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_DELETE_JOB);
    ret = rcl_service_fini(&g_serviceDeleteJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_DELETE_JOB " service: %d", ret);
    rosidl_runtime_c__String__fini(&g_messages_DeleteJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_delete_job_fini);
}

void Ros_ServiceDeleteJob_Trigger(const void* request_msg, void* response_msg)
{
    DeleteJobRequest const* request = (DeleteJobRequest*)request_msg;
    DeleteJobResponse* response = (DeleteJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to delete job with name: '%s'",
        __func__, RAW_CHAR_P(request->name));

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = -1;
    goto DONE;

    //request validation
    //TODO(gavanderhoorn): should we only allow this in REMOTE mode?

    if (Ros_strnlen(RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN - 1) == 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Empty job name not allowed");
        response->result_code = -1;
        goto DONE;
    }
    //check against some 'arbitrary' length longer than the allowed maximum
    if (Ros_strnlen(RAW_CHAR_P(request->name), 64) >= MAX_JOB_NAME_LEN)
    {
        rosidl_runtime_c__String__assign(&response->message, "Job name too long");
        response->result_code = -1;
        goto DONE;
    }

    //validated, attempt deletion
    MP_DELETE_JOB_SEND_DATA sData;
    MP_STD_RSP_DATA rData;

    //TODO(gavanderhoorn): check terminating nul
    //TODO(gavanderhoorn): check return value
    bzero(&sData, sizeof(sData));
    strncpy(sData.cJobName, RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN - 1);
    LONG status = mpDeleteJob(&sData, &rData);
    if (status != OK)
    {
        rosidl_runtime_c__String__assign(&response->message, "Error invoking M+ API");
        response->result_code = -2;
        goto DONE;
    }

    if (rData.err_no != 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Could not delete job");
        response->result_code = -rData.err_no;
        goto DONE;
    }

    rosidl_runtime_c__String__assign(&response->message, "Success");
    response->result_code = 1;

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
