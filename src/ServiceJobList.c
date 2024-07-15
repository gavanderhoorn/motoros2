//ServiceJobList.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

rcl_service_t g_serviceListJobs;

ServiceListJobs_Messages g_messages_ListJobs;

typedef motoros2_interfaces__srv__ListJobs_Response ListJobsResponse;


static const size_t RESP_NAMES_LIST_INITIAL_SIZE = 64;


void Ros_ServiceListJobs_Initialize()
{
    MOTOROS2_MEM_TRACE_START(svc_list_jobs_init);

    const rosidl_service_type_support_t* type_support =
        ROSIDL_GET_SRV_TYPE_SUPPORT(motoros2_interfaces, srv, ListJobs);

    rcl_ret_t ret = rclc_service_init_default(
        &g_serviceListJobs, &g_microRosNodeInfo.node, type_support, SERVICE_NAME_LIST_JOBS);
    motoRosAssert_withMsg(ret == RCL_RET_OK,
        SUBCODE_FAIL_INIT_SERVICE_LIST_JOBS, "Failed to init service (%d)", (int)ret);

    rosidl_runtime_c__String__init(&g_messages_ListJobs.response.message);
    //NOTE: if/when needed, this will be resized
    rosidl_runtime_c__String__Sequence__init(
        &g_messages_ListJobs.response.names, RESP_NAMES_LIST_INITIAL_SIZE);
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

    //make sure we don't accidentally return old results (in case of errors fi)
    rosidl_runtime_c__String__assign(&response->message, "");
    response->result_code = 0;
    response->names.size = 0;

    //retrieve list of jobs (".JBI")
    //TODO(gavanderhoorn): should this also look for other files (JBR)?
    Ros_Debug_BroadcastMsg("%s: updating job file list", __func__);
    LONG status = mpRefreshFileList(MP_EXT_ID_JBI);
    if (status != OK)
    {
        Ros_Debug_BroadcastMsg("%s: error refreshing job list", __func__);
        rosidl_runtime_c__String__assign(&response->message, "error refreshing job list");
        //TODO(gavanderhoorn): define status/error codes
        response->result_code = 10;
        goto DONE;
    }

    //see how many jobs there are, and if necessary, resize list of names in service response
    size_t job_count = mpGetFileCount();
    if (job_count < 0)
    {
        Ros_Debug_BroadcastMsg("%s: error retrieving file count: %d", __func__, job_count);
        rosidl_runtime_c__String__assign(&response->message, "error retrieving file count");
        //TODO(gavanderhoorn): define status/error codes
        response->result_code = 11;
        goto DONE;
    }
    Ros_Debug_BroadcastMsg("%s: found %d jobs", __func__, job_count);
    if (job_count > response->names.capacity)
    {
        Ros_Debug_BroadcastMsg("%s: resize needed: current: %d; need: %d",
            __func__, response->names.capacity, job_count);
        rosidl_runtime_c__String__Sequence__fini(&response->names);
        //TODO(gavanderhoorn): handle errors
        rosidl_runtime_c__String__Sequence__init(&response->names, job_count);
    }

    //copy all jobs on the list to the response
    //32 characters for the name, plus 4 for extension, plus terminating null
    //TODO(gavanderhoorn): is this really necessary? Copied from inform generator
    //TODO(gavanderhoorn): confirm whether extension is included or not
    char name_buf[MAX_JOB_NAME_LEN + 4 + 1] = { 0 };
    for (size_t i = 0; i < job_count; ++i)
    {
        //retrieve job name from internal list
        status = mpGetFileName(i, name_buf);
        if (status != OK)
        {
            Ros_Debug_BroadcastMsg("%s: failed retrieving job %d from list", __func__, i);
            rosidl_runtime_c__String__assign(&response->message, "failed retrieving job from list");
            //TODO(gavanderhoorn): define status/error codes
            response->result_code = 12;
            goto DONE;
        }

        //add it to the response
        if (!rosidl_runtime_c__String__assignn(
            &response->names.data[i], name_buf, MAX_JOB_NAME_LEN)
        )
        {
            Ros_Debug_BroadcastMsg("%s: failed adding job name %d to response, aborting", __func__, i);
            rosidl_runtime_c__String__assign(&response->message, "Error adding job name to response");
            //TODO(gavanderhoorn): define status/error codes
            response->result_code = 13;
            goto DONE;
        }
    }

    //finally update nr of items in list
    response->names.size = job_count;

    //done, report to client
    Ros_Debug_BroadcastMsg("%s: returning %d job names", __func__, response->names.size);
    rosidl_runtime_c__String__assign(&response->message, "success");
    //TODO(gavanderhoorn): define status/error codes
    response->result_code = 1;

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
