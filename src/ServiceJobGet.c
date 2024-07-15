//ServiceJobGet.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

#define RAW_CHAR_P(micro_ros_str) (micro_ros_str.data)

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
        __func__, RAW_CHAR_P(request->name));

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = 0;
    goto DONE;


    //request validation
    if (Ros_strnlen(RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN - 1) == 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Empty job name not allowed");
        response->result_code = 10;
        goto DONE;
    }
    //check against some 'arbitrary' length longer than the allowed maximum
    if (Ros_strnlen(RAW_CHAR_P(request->name), 64) >= MAX_JOB_NAME_LEN)
    {
        rosidl_runtime_c__String__assign(&response->message, "Job name too long");
        response->result_code = 11;
        goto DONE;
    }


    char jobNameWithExtension[_PARM_PATH_MAX];
    char pathDram[_PARM_PATH_MAX];
    int rv;
    int fdJob;

    snprintf(jobNameWithExtension, _PARM_PATH_MAX, "%s.%s", RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    snprintf(pathDram, _PARM_PATH_MAX, "%s\\%s", MP_DRAM_DEV_DOS, jobNameWithExtension);
    rv = mpSaveFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);

    if (rv != OK) //job doesn't exist; let's build one
    {
        Ros_Debug_BroadcastMsg("%s: error: job '%s' doesn't exist", __func__, pathDram);
        rosidl_runtime_c__String__assign(&response->message, "Job doesn't exist");
        response->result_code = 12;
        goto DONE;
    }

    fdJob = mpOpen(pathDram, 0, O_RDONLY);
    rv = mpClose(fdJob);

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
