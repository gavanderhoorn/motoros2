//ServiceInformJobDelete.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"
#include "ServiceInformJobShared.h"

rcl_service_t g_serviceDeleteInformJob;

ServiceDeleteInformJob_Messages g_messages_DeleteInformJob;

typedef motoros2_interfaces__srv__DeleteInformJob_Request DeleteInformJobRequest;
typedef motoros2_interfaces__srv__DeleteInformJob_Response DeleteInformJobResponse;


void Ros_ServiceDeleteInformJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_delete_inform_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, DeleteInformJob);

    rcl_ret_t ret = rclc_service_init_default(&g_serviceDeleteInformJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_DELETE_INFORM_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK,
        SUBCODE_FAIL_INIT_SERVICE_DELETE_INFORM_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_DeleteInformJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_delete_inform_job_init);
}

void Ros_ServiceDeleteInformJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_delete_inform_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_DELETE_INFORM_JOB);
    ret = rcl_service_fini(&g_serviceDeleteInformJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_DELETE_INFORM_JOB " service: %d", ret);
    rosidl_runtime_c__String__fini(&g_messages_DeleteInformJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_delete_inform_job_fini);
}

static bool ValidateRequest(DeleteInformJobRequest const* request, DeleteInformJobResponse* const response)
{
    if (Ros_strnlen(RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN - 1) == 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Empty job name not allowed");
        response->result_code = 11;
        return false;
    }
    //check against some 'arbitrary' length longer than the allowed maximum
    if (Ros_strnlen(RAW_CHAR_P(request->name), 64) >= MAX_JOB_NAME_LEN)
    {
        rosidl_runtime_c__String__assign(&response->message, "Job name too long");
        response->result_code = 12;
        return false;
    }

    return true;
}

void Ros_ServiceDeleteInformJob_Trigger(const void* request_msg, void* response_msg)
{
    //TODO(gavanderhoorn): should we only allow this in REMOTE mode?

    DeleteInformJobRequest const* request = (DeleteInformJobRequest*)request_msg;
    DeleteInformJobResponse* response = (DeleteInformJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to delete job with name: '%s'",
        __func__, RAW_CHAR_P(request->name));

    response->result_code = 0;
    rosidl_runtime_c__String__assign(&response->message, "");

    //request validation
    if (!ValidateRequest(request, response))
    {
        Ros_Debug_BroadcastMsg("%s: request validation failed, aborting", __func__);
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
        Ros_Debug_BroadcastMsg("%s: error deleting job: %d (0x%04X)", __func__, status, rData.err_no);
        rosidl_runtime_c__String__assign(&response->message, "M+ API error: can't delete job");
        response->result_code = 13;
        goto DONE;
    }

    if (rData.err_no != 0)
    {
        Ros_Debug_BroadcastMsg("%s: error deleting job: 0x%04X", __func__, rData.err_no);
        rosidl_runtime_c__String__assign(&response->message, "Could not delete job");
        response->result_code = -rData.err_no;
        goto DONE;
    }

    rosidl_runtime_c__String__assign(&response->message, "Success");
    response->result_code = 1;

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
