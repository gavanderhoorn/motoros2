//ServiceJobPut.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

#define MAX_JOB_NAME_LENGTH_WITH_EXTENSION (MAX_JOB_NAME_LEN + 4 + 1)   //32 characters for the name, plus 4 for extension, plus terminating null
//TODO(gavanderhoorn): 
#define MAX_JOB_FILE_SIZE (1024)  //in bytes

#define RAW_CHAR_P(micro_ros_str) (micro_ros_str.data)


rcl_service_t g_servicePutJob;

ServicePutJob_Messages g_messages_PutJob;

typedef motoros2_interfaces__srv__PutJob_Request PutJobRequest;
typedef motoros2_interfaces__srv__PutJob_Response PutJobResponse;


void Ros_ServicePutJob_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_put_job_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, PutJob);

    rcl_ret_t ret = rclc_service_init_default(&g_servicePutJob,
        &g_microRosNodeInfo.node, type_support, SERVICE_NAME_PUT_JOB);
    motoRosAssert_withMsg(ret == RCL_RET_OK, 
        SUBCODE_FAIL_INIT_SERVICE_PUT_JOB, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_PutJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_put_job_init);
}

void Ros_ServicePutJob_Cleanup()
{
    rcl_ret_t ret;
    MOTOROS2_MEM_TRACE_START(svc_put_job_fini);

    Ros_Debug_BroadcastMsg("Cleanup service " SERVICE_NAME_PUT_JOB);
    ret = rcl_service_fini(&g_servicePutJob, &g_microRosNodeInfo.node);
    if (ret != RCL_RET_OK)
        Ros_Debug_BroadcastMsg("Failed cleaning up " SERVICE_NAME_PUT_JOB " service: %d", ret);
    rosidl_runtime_c__String__fini(&g_messages_PutJob.response.message);

    MOTOROS2_MEM_TRACE_REPORT(svc_put_job_fini);
}

void Ros_ServicePutJob_Trigger(const void* request_msg, void* response_msg)
{
    PutJobRequest const* request = (PutJobRequest*)request_msg;
    PutJobResponse* response = (PutJobResponse*)response_msg;

    Ros_Debug_BroadcastMsg("%s: enter", __func__);

    Ros_Debug_BroadcastMsg("%s: request to create job with name: '%s'",
        __func__, RAW_CHAR_P(request->name));

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = -1;
    goto DONE;


    //request validation
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
    //an empty job is definitely not OK
    //TODO(gavanderhoorn): what is the minimum size of a complete job?
    if (Ros_strnlen(RAW_CHAR_P(request->content), MAX_JOB_FILE_SIZE) < 50)
    {
        rosidl_runtime_c__String__assign(&response->message, "Not enough data for job (check 'content' field)");
        response->result_code = -1;
        goto DONE;
    }

    //validated, attempt to create the job
    char pathToGeneratedJob[_PARM_PATH_MAX];
    int fd;
    int ret;

    //create job on DRAM
    sprintf(pathToGeneratedJob, "%s\\%s.%s", MP_DRAM_DEV_DOS, RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    Ros_Debug_BroadcastMsg("%s: creating INFORM job at: '%s'", __func__, pathToGeneratedJob);

    if ((fd = mpCreate(pathToGeneratedJob, O_WRONLY)) < 0)
    {
        Ros_Debug_BroadcastMsg("%s: failed to create job on DRAM drive: %x",__func__, fd);
        rosidl_runtime_c__String__assign(&response->message, "Could not create job on DRAM drive");
        response->result_code = -1;
        goto DONE;
    }

    //write contents
    size_t num_bytes = Ros_strnlen(RAW_CHAR_P(request->content), MAX_JOB_FILE_SIZE);
    ret = mpWrite(fd, RAW_CHAR_P(request->content), num_bytes);
    if (ret < num_bytes)
    {
        Ros_Debug_BroadcastMsg("%s: failed to write job contents: %x", __func__, ret);
        rosidl_runtime_c__String__assign(&response->message, "Could not write job contents to file");
        response->result_code = -1;
        goto DONE;
    }
    mpClose(fd);

    //load job
    char jobNameWithExtension[MAX_JOB_NAME_LENGTH_WITH_EXTENSION];
    snprintf(jobNameWithExtension, MAX_JOB_NAME_LENGTH_WITH_EXTENSION, "%s.%s", RAW_CHAR_P(request->name), MP_EXT_STR_JBI);
    Ros_Debug_BroadcastMsg("%s: loading INFORM job from: '%s'", __func__, jobNameWithExtension);

    ret = mpLoadFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);
    if (ret != OK)
    {
        Ros_Debug_BroadcastMsg("%s: could not load job to CMOS: 0x%04X", __func__, ret);
        rosidl_runtime_c__String__assign(&response->message, "Could not load job to CMOS");
        response->result_code = -2;
        goto DONE;
    }

    //success
    Ros_Debug_BroadcastMsg("%s: loaded INFORM job", __func__);
    rosidl_runtime_c__String__assign(&response->message, "Loaded INFORM job");
    response->result_code = 1;

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
