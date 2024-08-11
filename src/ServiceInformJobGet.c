//ServiceInformJobGet.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"
#include "ServiceInformJobShared.h"

rcl_service_t g_serviceGetInformJob;

ServiceGetInformJob_Messages g_messages_GetInformJob;

typedef motoros2_interfaces__srv__GetInformJob_Request GetInformJobRequest;
typedef motoros2_interfaces__srv__GetInformJob_Response GetInformJobResponse;


void Ros_ServiceGetInformJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_get_inform_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, GetInformJob);

    rcl_ret_t ret = rclc_service_init_default(&g_serviceGetInformJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_GET_INFORM_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK,
        SUBCODE_FAIL_INIT_SERVICE_GET_INFORM_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_GetInformJob.response.message);
    rosidl_runtime_c__byte__Sequence__init(&g_messages_GetInformJob.response.contents, MAX_JOB_FILE_SIZE);
    g_messages_GetInformJob.response.contents.size = 0;

    MOTOROS2_MEM_TRACE_REPORT(svc_get_inform_job_init);
}

void Ros_ServiceGetInformJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_get_inform_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_GET_INFORM_JOB);
    ret = rcl_service_fini(&g_serviceGetInformJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_GET_INFORM_JOB " service: %d", ret);
    rosidl_runtime_c__byte__Sequence__fini(&g_messages_GetInformJob.response.contents);
    rosidl_runtime_c__String__fini(&g_messages_GetInformJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_get_inform_job_fini);
}

static bool ValidateRequest(GetInformJobRequest const* request, GetInformJobResponse* const response)
{
    if (Ros_strnlen(RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN - 1) == 0)
    {
        rosidl_runtime_c__String__assign(&response->message, "Empty job name not allowed");
        response->result_code = 10;
        return false;
    }

    //check against some 'arbitrary' length longer than the allowed maximum
    if (Ros_strnlen(RAW_CHAR_P(request->name), MAX_JOB_NAME_LEN + 10) >= MAX_JOB_NAME_LEN)
    {
        rosidl_runtime_c__String__assign(&response->message, "Job name too long");
        response->result_code = 11;
        return false;
    }

    return true;
}

void Ros_ServiceGetInformJob_Trigger(const void* request_msg, void* response_msg)
{
    GetInformJobRequest const* request = (GetInformJobRequest*)request_msg;
    GetInformJobResponse* response = (GetInformJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to return job with name: '%s'",
        __func__, RAW_CHAR_P(request->name));

    //assume we'll return nothing
    response->contents.size = 0;
    response->result_code = 0;
    rosidl_runtime_c__String__assign(&response->message, "");

    //request validation
    if (!ValidateRequest(request, response))
    {
        Ros_Debug_BroadcastMsg("%s: request validation failed, aborting", __func__);
        goto DONE;
    }

    //validation OK (enough): attempt to copy job file contents into service response
    char jobNameWithExtension[_PARM_PATH_MAX] = { 0 };
    char pathDram[_PARM_PATH_MAX] = { 0 };
    snprintf(jobNameWithExtension, _PARM_PATH_MAX, "%s.%s", RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    snprintf(pathDram, _PARM_PATH_MAX, "%s\\%s", MP_DRAM_DEV_DOS, jobNameWithExtension);
    mpRemove(pathDram);
    Ros_Debug_BroadcastMsg("%s: copying '%s' from SRAM to '%s'", __func__, jobNameWithExtension, pathDram);

    //copy job from SRAM to RAM drive (anything != OK we interpret as: job doesn't exist)
    int ret = mpSaveFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);
    if (ret != OK)
    {
        Ros_Debug_BroadcastMsg("%s: error copying job '%s': 0x%04X", __func__, pathDram, ret);
        rosidl_runtime_c__String__assign(&response->message, "Job doesn't exist");
        response->result_code = 12;
        goto DONE;
    }

    //attempt to open the job file on the RAM drive
    int fdJob = -1;
    if ((fdJob = mpOpen(pathDram, O_RDONLY, 0)) < 0)
    {
        Ros_Debug_BroadcastMsg("%s: error: couldn't open '%s': %d", __func__, pathDram, fdJob);
        rosidl_runtime_c__String__assign(&response->message, "Can't open job file");
        response->result_code = 13;
        goto DONE;
    }

    //try to determine actual file size
    struct stat pStat = { 0 };
    if ((ret = mpFstat(fdJob, &pStat)) < 0)
    {
        Ros_Debug_BroadcastMsg("%s: error: couldn't fstat '%s': %d", __func__, pathDram, ret);
        rosidl_runtime_c__String__assign(&response->message, "Error determining job file size");
        response->result_code = 14;
        goto DONE_W_FD;
    }

    //we artifically limit the maximum size of jobs we can manage, so check
    if (pStat.st_size >= MAX_JOB_FILE_SIZE)
    {
        Ros_Debug_BroadcastMsg("%s: error: '%s' too big: %d bytes (max %d)",
            __func__, pathDram, pStat.st_size, MAX_JOB_FILE_SIZE);
        rosidl_runtime_c__String__assign(&response->message, "Job content exceeds maximum file size");
        response->result_code = 15;
        goto DONE_W_FD;
    }

    //attempt to copy job contents into the service response directly
    //NOTE: 'response->contents.data' is initialised previously to be of at least
    //'pStat.st_size' size and we can't get here unless the job contents is smaller
    //than the maximum we support (ie: MAX_JOB_FILE_SIZE)
    ret = mpRead(fdJob, (char*) response->contents.data , /*maxBytes=*/pStat.st_size);
    if (ret < 0)
    {
        Ros_Debug_BroadcastMsg("%s: error reading '%s': %d", __func__, pathDram, ret);
        rosidl_runtime_c__String__assign(&response->message, "Couldn't read job file contents");
        response->result_code = 16;
        goto DONE_W_FD;
    }
    else if (ret < pStat.st_size)
    {
        Ros_Debug_BroadcastMsg("%s: error reading '%s': read only %d of %d bytes",
            __func__, pathDram, ret, pStat.st_size);
        rosidl_runtime_c__String__assign(&response->message, "Couldn't read complete job contents");
        response->result_code = 17;
        goto DONE_W_FD;
    }

    //finally: store nr of bytes we'll be returning
    response->contents.size = pStat.st_size;

    //complete successful response
    rosidl_runtime_c__String__assign(&response->message, "Success");
    response->result_code = 1;

    Ros_Debug_BroadcastMsg("%s: returning %d bytes for job '%s'",
        __func__, pStat.st_size, RAW_CHAR_P(request->name));

DONE_W_FD:
    mpClose(fdJob);

DONE:
    mpRemove(pathDram);

    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
