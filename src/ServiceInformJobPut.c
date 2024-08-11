//ServiceInformJobPut.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"
#include "ServiceInformJobShared.h"

rcl_service_t g_servicePutInformJob;

ServicePutInformJob_Messages g_messages_PutInformJob;

typedef motoros2_interfaces__srv__PutInformJob_Request PutInformJobRequest;
typedef motoros2_interfaces__srv__PutInformJob_Response PutInformJobResponse;


void Ros_ServicePutInformJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_put_inform_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, PutInformJob);

    rcl_ret_t ret = rclc_service_init_default(&g_servicePutInformJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_PUT_INFORM_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK,
        SUBCODE_FAIL_INIT_SERVICE_PUT_INFORM_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_PutInformJob.response.message);
    rosidl_runtime_c__byte__Sequence__init(&g_messages_PutInformJob.request.contents, MAX_JOB_FILE_SIZE);

    MOTOROS2_MEM_TRACE_REPORT(svc_put_inform_job_init);
}

void Ros_ServicePutInformJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_put_inform_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_PUT_INFORM_JOB);
    ret = rcl_service_fini(&g_servicePutInformJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_PUT_INFORM_JOB " service: %d", ret);
    rosidl_runtime_c__byte__Sequence__fini(&g_messages_PutInformJob.request.contents);
    rosidl_runtime_c__String__fini(&g_messages_PutInformJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_put_inform_job_fini);
}

static bool ValidateRequest(PutInformJobRequest const* request, PutInformJobResponse* const response)
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

    //an empty job is definitely not OK
    //TODO(gavanderhoorn): what is the minimum size of a complete job?
    if (request->contents.size < 50)
    {
        rosidl_runtime_c__String__assign(&response->message, "Not enough data for job (check 'content' field)");
        response->result_code = 12;
        return false;
    }

    //we artifically limit the maximum size of jobs we can manage, so check
    //TODO(gavanderhoorn): this is probably impossible, as msgs that don't fit
    //in the pre-allocated buffer will not be seen by this code
    if (request->contents.size >= MAX_JOB_FILE_SIZE)
    {
        Ros_Debug_BroadcastMsg("%s: error: '%s' too big: %d bytes (max %d)",
            __func__, RAW_CHAR_P(request->name), request->contents.size, MAX_JOB_FILE_SIZE);
        rosidl_runtime_c__String__assign(&response->message, "Job content exceeds maximum file size");
        response->result_code = 13;
        return false;
    }

    return true;
}

void Ros_ServicePutInformJob_Trigger(const void* request_msg, void* response_msg)
{
    PutInformJobRequest const* request = (PutInformJobRequest*)request_msg;
    PutInformJobResponse* response = (PutInformJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to create job with name: '%s'",
        __func__, RAW_CHAR_P(request->name));

    response->result_code = 0;
    rosidl_runtime_c__String__assign(&response->message, "");

    //request validation
    if (!ValidateRequest(request, response))
    {
        Ros_Debug_BroadcastMsg("%s: request validation failed, aborting", __func__);
        goto DONE;
    }

    //validation OK (enough): attempt to create the job (first on RAM drive, then load it to SRAM)
    char newJobPath[_PARM_PATH_MAX];
    sprintf(newJobPath, "%s\\%s.%s", MP_DRAM_DEV_DOS, RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    Ros_Debug_BroadcastMsg("%s: creating INFORM job at: '%s'", __func__, newJobPath);

    int fd;
    if ((fd = mpCreate(newJobPath, O_WRONLY)) < 0)
    {
        Ros_Debug_BroadcastMsg("%s: failed to create job on RAM drive: %d",__func__, fd);
        rosidl_runtime_c__String__assign(&response->message, "Could not create job on RAM drive");
        response->result_code = 14;
        goto DONE;
    }

    //attempt to write contents to temp file on RAM drive
    int ret = mpWrite(fd, (char*) request->contents.data, request->contents.size);
    if (ret < 0)
    {
        Ros_Debug_BroadcastMsg("%s: failed to write job contents: %d", __func__, ret);
        rosidl_runtime_c__String__assign(&response->message, "Could not write job contents to file");
        response->result_code = 15;
        goto DONE_W_FD;
    }
    else if (ret < request->contents.size)
    {
        Ros_Debug_BroadcastMsg("%s: error writing '%s': wrote only %d of %d bytes",
            __func__, newJobPath, ret, request->contents.size);
        rosidl_runtime_c__String__assign(&response->message, "Could not write complete job contents to file");
        response->result_code = 16;
        goto DONE_W_FD;
    }

    //close fd as we've created the job file on the RAM drive
    mpClose(fd);

    //now load the job to SRAM from the RAM drive
    char jobNameWithExtension[MAX_JOB_NAME_LENGTH_WITH_EXTENSION];
    snprintf(jobNameWithExtension, MAX_JOB_NAME_LENGTH_WITH_EXTENSION, "%s.%s", RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    Ros_Debug_BroadcastMsg("%s: loading INFORM job from: '%s'", __func__, jobNameWithExtension);

    ret = mpLoadFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);
    if (ret != OK)
    {
        Ros_Debug_BroadcastMsg("%s: could not load job to CMOS: 0x%04X", __func__, ret);
        rosidl_runtime_c__String__assign(&response->message, "Could not load job to CMOS");
        response->result_code = 17;
        goto DONE;
    }

    //complete successful response
    rosidl_runtime_c__String__assign(&response->message, "Success");
    response->result_code = 1;

    Ros_Debug_BroadcastMsg("%s: loaded INFORM job (%d bytes)", __func__, request->contents.size);

DONE_W_FD:
    mpClose(fd);

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
