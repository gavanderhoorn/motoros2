//ServiceJobPut.c

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

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

    rosidl_runtime_c__String__assign(&response->message, "Not implemented");
    response->result_code = 0;

DONE:
    Ros_Debug_BroadcastMsg("%s: exit", __func__);
}
