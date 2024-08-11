//ServiceInformJobShared.h

// SPDX-FileCopyrightText: 2024, Yaskawa America, Inc.
// SPDX-FileCopyrightText: 2024, Delft University of Technology
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MOTOROS2_SERVICE_INFORM_JOB_SHARED_H
#define MOTOROS2_SERVICE_INFORM_JOB_SHARED_H


#define MAX_JOB_FILE_SIZE (4 * 1024)
#define RAW_CHAR_P(micro_ros_str) (micro_ros_str.data)

//32 characters for the name, plus 4 for extension, plus terminating null
#define MAX_JOB_NAME_LENGTH_WITH_EXTENSION (MAX_JOB_NAME_LEN + 4 + 1)


#endif  // MOTOROS2_SERVICE_INFORM_JOB_SHARED_H
