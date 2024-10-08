//InformCheckerAndGenerator.c

// SPDX-FileCopyrightText: 2022-2023, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2022-2023, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#include "MotoROS.h"

#define MAX_JOB_LINE_LENGTH 512
#define MAX_JOB_NAME_LENGTH_WITH_EXTENSION (MAX_JOB_NAME_LEN + 4 + 1)   //32 characters for the name, plus 4 for extension, plus terminating null

const char* rosInitJobLines[] =
{
    "NOP",
    "DOUT OT#(890) OFF",
    "DOUT OT#(889) OFF",
#if defined(YRC1000) || defined(DX200) //DX2 and YRC use two digit precision on the TIMER
    "TIMER T=0.05",
#elif defined(YRC1000u) || defined(FS100) //FS and YRCu use three digit precision on the TIMER
    "TIMER T=0.050",
#else
#error Validate the precision of the TIMER instruction on the teach pendant and update this accordingly
#endif
    "DOUT OT#(889) ON",
    "WAIT OT#(890)=ON",
    "DOUT OT#(890) OFF",
    "DOUT OT#(889) OFF",
    "END",
};

void Ros_InformChecker_CreateJob()
{
    char lineBuffer[MAX_JOB_LINE_LENGTH];
    char tagBuffer[32];
    char pathToGeneratedJob[_PARM_PATH_MAX];
    int fd;
    MP_CALENDAR_RSP_DATA calendar;

    //create job on DRAM
    sprintf(pathToGeneratedJob, "%s\\%s.JBI", MP_DRAM_DEV_DOS, g_nodeConfigSettings.inform_job_name);
    fd = mpCreate(pathToGeneratedJob, O_WRONLY);

    Ros_Debug_BroadcastMsg("Generating INFORM job in temporary location '%s'", pathToGeneratedJob);

    if (fd < 0)
    {
        mpSetAlarm(ALARM_INFORM_JOB_FAIL, APPLICATION_NAME " failed to generate job", SUBCODE_INFORM_FAIL_TO_CREATE_JOB);
        Ros_Debug_BroadcastMsg("Could not create job on DRAM drive");
        return;
    }

    //================
    // Write header
    //================
    FileUtilityFunctions_WriteLine(fd, "/JOB");
    FileUtilityFunctions_WriteLine(fd, "//NAME %s", g_nodeConfigSettings.inform_job_name);
    FileUtilityFunctions_WriteLine(fd, "//POS");
    FileUtilityFunctions_WriteLine(fd, "///NPOS 0,0,0,0,0,0");
    FileUtilityFunctions_WriteLine(fd, "//INST");
    mpGetCalendar(&calendar);
    FileUtilityFunctions_WriteLine(fd, "///DATE %d/%02d/%02d %02d:%02d", calendar.usYear, calendar.usMonth, calendar.usDay, calendar.usHour, calendar.usMin);
    FileUtilityFunctions_WriteLine(fd, "///COMM AUTO-GENERATED BY " APPLICATION_NAME);
    FileUtilityFunctions_WriteLine(fd, "///ATTR SC,RW");

    //================
    // Write control groups
    //================
    for (int groupIndex = 0, groupCombination = 1; groupIndex < g_Ros_Controller.numGroup; groupIndex += 1)
    {
        CtrlGroup* group = g_Ros_Controller.ctrlGroups[groupIndex];
        if (group)
        {
            if (Ros_CtrlGroup_IsRobot(group))
            {
                snprintf(lineBuffer, MAX_JOB_LINE_LENGTH, "///GROUP%d RB%d", groupCombination, (group->groupId - MP_R1_GID) + 1);
                groupCombination += 1;

                if (group->baseTrackGroupId != -1) //this robot has a base
                {
                    sprintf(tagBuffer, ",BS%d", (group->baseTrackGroupId - MP_B1_GID) + 1);
                    strncat(lineBuffer, tagBuffer,
                        sizeof(lineBuffer) - Ros_strnlen(lineBuffer, MAX_JOB_LINE_LENGTH) - 1);
                }

                FileUtilityFunctions_WriteLine(fd, lineBuffer);
            }
            else if (group->groupId >= MP_S1_GID) //is a station axis
            {
                sprintf(lineBuffer, "///GROUP%d ST%d", groupCombination, (group->groupId - MP_S1_GID) + 1);
                groupCombination += 1;

                FileUtilityFunctions_WriteLine(fd, lineBuffer);
            }
        }
    }

    //================
    // Write job contents
    //================
    int numberOfLines = sizeof(rosInitJobLines) / sizeof(rosInitJobLines[0]);
    for (int i = 0; i < numberOfLines; i += 1)
    {
        FileUtilityFunctions_WriteLine(fd, rosInitJobLines[i]);
    }

    mpClose(fd);

    //================
    // Upload job
    //================
    MP_DELETE_JOB_SEND_DATA delSendData;
    MP_STD_RSP_DATA stdResponseData;
    char jobNameWithExtension[MAX_JOB_NAME_LENGTH_WITH_EXTENSION];
    int rv;

    strncpy(delSendData.cJobName, g_nodeConfigSettings.inform_job_name, MAX_JOB_NAME_LEN);
    mpDeleteJob(&delSendData, &stdResponseData);

    snprintf(jobNameWithExtension, MAX_JOB_NAME_LENGTH_WITH_EXTENSION, "%s.JBI", g_nodeConfigSettings.inform_job_name);
    rv = mpLoadFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);

    if (rv != OK)
    {
        mpSetAlarm(ALARM_INFORM_JOB_FAIL, APPLICATION_NAME " failed to load job", SUBCODE_INFORM_FAIL_TO_LOAD_JOB);
        Ros_Debug_BroadcastMsg("Could not load job to CMOS: 0x%04X", rv);
    }
    else
    {
        Ros_Debug_BroadcastMsg("Generated INFORM job, loaded as '%s'", g_nodeConfigSettings.inform_job_name);
    }
}

BOOL Ros_InformChecker_CheckHeaderForGroupCombinations(int fdJob)
{
    char lineBuffer[MAX_JOB_LINE_LENGTH];
    rcutils_string_array_t splitSpace = rcutils_get_zero_initialized_string_array();
    rcutils_string_array_t splitComma = rcutils_get_zero_initialized_string_array();
    int ret;

    const int MAX_GROUP_COMBINATIONS = 4;
    const int MAX_GROUP_ITEMS_IN_COMBINATION = 3;
    MP_GRP_ID_TYPE groupCombinations[MAX_GROUP_COMBINATIONS][MAX_GROUP_ITEMS_IN_COMBINATION];

    for (int i = 0; i < MAX_GROUP_COMBINATIONS; i += 1)
    {
        for (int j = 0; j < MAX_GROUP_ITEMS_IN_COMBINATION; j += 1)
        {
            groupCombinations[i][j] = (MP_GRP_ID_TYPE)-1;
        }
    }

    //================
    // Read header
    //================
    //
    // Figure out which groups are used in this JBI
    while (FileUtilityFunctions_ReadLine(fdJob, lineBuffer, MAX_JOB_LINE_LENGTH))
    {
        if (strncmp(lineBuffer, "NOP", MAX_JOB_LINE_LENGTH) == 0)
        {
            break; //done parsing header
        }

        //capture the control groups which are used within this job
        else if (strstr(lineBuffer, "///GROUP"))
        {
            int groupSubNum = 0;

            //seperates "///GROUPx" from the rest of the string
            ret = rcutils_split(lineBuffer, ' ', g_motoros2_Allocator, &splitSpace);
            if (ret != RCUTILS_RET_OK)
            {
                Ros_Debug_BroadcastMsg("Failed to parse the GROUP label from the indvidual tags (%d)", ret);
                break;
            }

            //Isolate the number after the word GROUP
            //Example: ///GROUP1 RB1
            //This will get the first 1
            int groupCombinationIndex = atoi(&splitSpace.data[0][strlen("///GROUP")]) - 1;

            //split the group tags
            ret = rcutils_split(splitSpace.data[1], ',', g_motoros2_Allocator, &splitComma);
            if (ret != RCUTILS_RET_OK)
            {
                Ros_Debug_BroadcastMsg("Failed to parse the GROUP tags (%d)", ret);
                break;
            }

            //convert the group tags to MP_GRP_ID_TYPE
            for (int i = 0; i < splitComma.size; i += 1)
            {
                int offset = 0;

                if (strstr(splitComma.data[i], "RB"))
                    offset = 0;
                else if (strstr(splitComma.data[i], "BS"))
                    offset = MP_B1_GID;
                else if (strstr(splitComma.data[i], "ST"))
                    offset = MP_S1_GID;

                //Isolate the number after group identifier tag
                //Example: ///GROUP1 RB1
                //This will get the second 1 and add it to the 'offset' above
                groupCombinations[groupCombinationIndex][groupSubNum] = (MP_GRP_ID_TYPE)((atoi(splitComma.data[i] + 2) - 1) + offset);
                groupSubNum += 1;
            }
        }
    }

    ret = rcutils_string_array_fini(&splitSpace);
    RCUTILS_UNUSED(ret);
    ret = rcutils_string_array_fini(&splitComma);
    RCUTILS_UNUSED(ret);

    //================
    // Make sure all groups are used are controlled by this robot job.
    //================
    for (int groupIndex = 0; groupIndex < g_Ros_Controller.numGroup; groupIndex += 1)
    {
        CtrlGroup* group = g_Ros_Controller.ctrlGroups[groupIndex];
        if (group)
        {
            BOOL bGroupFound = FALSE;
            //iterate over the used groups
            for (int i = 0; i < MAX_GROUP_COMBINATIONS; i += 1)
            {
                for (int j = 0; j < MAX_GROUP_ITEMS_IN_COMBINATION; j += 1)
                {
                    if (groupCombinations[i][j] == group->groupId)
                    {
                        bGroupFound = TRUE;
                        break;
                    }
                }
            }

            if (!bGroupFound)
                return FALSE;
        }
    }

    return TRUE;
}

BOOL Ros_InformChecker_CheckInformLines(int fdJob)
{
    char lineBuffer[MAX_JOB_LINE_LENGTH];
    int lineIndex = 0;
    int maxLines = sizeof(rosInitJobLines) / sizeof(rosInitJobLines[0]);
    BOOL bJobOk = TRUE;

    //================
    // Read contents
    //================
    lineIndex = 1; //fd is just past the NOP

    while (FileUtilityFunctions_ReadLine(fdJob, lineBuffer, MAX_JOB_LINE_LENGTH))
    {
        //too many lines in the job
        if (lineIndex >= maxLines)
        {
            bJobOk = FALSE;
            break;
        }

        //instruction line doesn't match
        if (strncmp(lineBuffer, rosInitJobLines[lineIndex], MAX_JOB_LINE_LENGTH) != 0)
        {
            //note: job lines are 0-indexed
            Ros_Debug_BroadcastMsg("%s: mismatch: line %d, expected: '%s', got: '%s'",
                __func__, lineIndex, rosInitJobLines[lineIndex], lineBuffer);
            bJobOk = FALSE;
            break;
        }

        lineIndex += 1;
    }

    return bJobOk;
}

void Ros_InformChecker_ValidateJob()
{
    char jobNameWithExtension[_PARM_PATH_MAX];
    char pathDram[_PARM_PATH_MAX];
    int rv;
    int fdJob;

    Ros_Debug_BroadcastMsg("Checking validity of robot job: %s", g_nodeConfigSettings.inform_job_name);

    snprintf(jobNameWithExtension, _PARM_PATH_MAX, "%s.JBI", g_nodeConfigSettings.inform_job_name);
    snprintf(pathDram, _PARM_PATH_MAX, "%s\\%s", MP_DRAM_DEV_DOS, jobNameWithExtension);

    mpRemove(pathDram); //don't care if this fails

    rv = mpSaveFile(MP_DRV_ID_DRAM, "", jobNameWithExtension);

    if (rv != OK) //job doesn't exist; let's build one
    {
        Ros_Debug_BroadcastMsg("Job file doesn't exist");
        Ros_InformChecker_CreateJob();
    }
    else
    {
        //Don't check the job contents if the user is allowing custom inform
        if (!g_nodeConfigSettings.allow_custom_inform_job)
        {
            fdJob = mpOpen(pathDram, O_RDONLY, 0);

            if (fdJob < 0)
                mpSetAlarm(ALARM_INFORM_JOB_FAIL, APPLICATION_NAME " failed to validate job", SUBCODE_INFORM_FAIL_TO_OPEN_DRAM);

            BOOL bJobOk = Ros_InformChecker_CheckHeaderForGroupCombinations(fdJob);

            if (!bJobOk)
                Ros_Debug_BroadcastMsg("Job header has invalid group combinations");

            if (bJobOk)
                bJobOk = Ros_InformChecker_CheckInformLines(fdJob);

            if (!bJobOk)
                Ros_Debug_BroadcastMsg("Job has invalid contents");

            mpClose(fdJob);

            //================
            // Do not fix the job automatically, just notify the user
            //================
            if (!bJobOk)
            {
                Ros_Debug_BroadcastMsg("Job exists, but is invalid.");
                mpSetAlarm(ALARM_INFORM_JOB_FAIL, "Invalid " APPLICATION_NAME " job detected", SUBCODE_INFORM_INVALID_JOB);
            }
            else
                Ros_Debug_BroadcastMsg("Job is OK");
        }
        else
            Ros_Debug_BroadcastMsg("Skipping validation of robot job '%s'. allow_custom_inform_job is true", g_nodeConfigSettings.inform_job_name);
    }
}
