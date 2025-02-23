#ifndef __TASK_LOG_H
#define __TASK_LOG_H

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include "Srv_OsCommon.h"
#include "imu_data.h"

#define LOG_HEADER 0xBA
#define LOG_DATATYPE_IMU 0x00

#define LOG_HEADER_SIZE sizeof(LogData_Header_TypeDef)

typedef enum
{
    Log_None_Halt = 0,
    Log_CompessFunc_Halt,
    Log_CompessSize_Halt,
    Log_DiskOprError_Halt,
    Log_Finish_Halt,
}Log_halt_Type;

typedef union
{
    struct
    {
        uint16_t IMU_Sec : 1;
        uint16_t Res_Sec : 15;
    } _sec;

    uint16_t reg_val;
} LogData_Reg_TypeDef;

#pragma pack(1)
typedef struct
{
    uint8_t header;
    uint8_t type;
    uint8_t size;
} LogData_Header_TypeDef;

typedef struct
{
    uint64_t time;
    uint8_t cyc;
    float acc_scale;
    float gyr_scale;
    uint16_t org_acc[Axis_Sum];
    uint16_t org_gyr[Axis_Sum];
    uint16_t flt_acc[Axis_Sum];
    uint16_t flt_gyr[Axis_Sum];

    uint8_t const_res[30];

    uint8_t check_sum;
}LogIMUData_TypeDef;

typedef struct
{
    uint32_t queue_push_err_cnt;
    uint32_t queue_pop_cnt;
    uint32_t compess_cnt;
    uint32_t write_file_cnt;
    uint32_t log_byte_sum;

    uint32_t uncompress_byte_sum;

    Log_halt_Type halt_type;
}Log_Statistics_TypeDef;

typedef union
{
    uint8_t buff[sizeof(LogIMUData_TypeDef)];
    LogIMUData_TypeDef data;
}LogIMUDataUnion_TypeDef;
#pragma pack()

void TaskLog_Init(uint32_t period);
void TaskLog_Core(void const *arg);

#endif
