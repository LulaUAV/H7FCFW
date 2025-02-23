#ifndef __STORAGE_H
#define __STORAGE_H

#include <string.h>
#include "Srv_DataHub.h"
#include "Bsp_Flash.h"
#include "Bsp_GPIO.h"
#include "Dev_W25Qxx.h"
#include "Srv_OsCommon.h"
#include "util.h"

#define Storage_Assert(x) while(x)

#define Format_Retry_Cnt 5
#define ExternalModule_ReInit_Cnt 5

#define Storage_ErrorCode_ToStr(x) #x

#define From_Start_Address 0

#define BootSection_Block_Size (4 Kb)
#define BootTab_Num 1

#define Storage_OnChip_Max_Capacity 256
#define Storage_ReserveBlock_Size 128

#define Storage_ExtFlash_Max_Capacity (1 Kb)

#define StorageItem_Size sizeof(Storage_Item_TypeDef)

#define INTERNAL_STORAGE_PAGE_TAG "[InternalFlash Storage]"
#define EXTERNAL_STORAGE_PAGE_TAG "[ExternalFlash Storage]"
#define INTERNAL_PAGE_TAG_SIZE strlen(INTERNAL_STORAGE_PAGE_TAG)
#define EXTERNAL_PAGE_TAG_SIZE strlen(EXTERNAL_STORAGE_PAGE_TAG)

#define STORAGE_NAME_LEN 41
#define STORAGE_ITEM_HEAD_TAG 0xAA
#define STORAGE_ITEM_END_TAG 0xBB
#define STORAGE_SLOT_HEAD_TAG 0xEF0110EF
#define STORAGE_SLOT_END_TAG 0xFE1001FE
#define STORAGE_DATA_ALIGN 4
#define STORAGE_MIN_BYTE_SIZE 1
#define STORAGE_FREEITEM_NAME "Item_Avaliable"

typedef uint32_t storage_handle;

typedef enum
{
    Storage_ChipBus_None = 0,
    Storage_ChipBus_Spi,
} Storage_ExtFlash_BusType_List;

typedef enum
{
    Storage_Chip_None = 0,
    Storage_ChipType_W25Qxx,
} Storage_ExtFlashChipType_List;

typedef enum
{
    Storage_Error_None = 0,
    Storage_Param_Error,
    Storage_BusInit_Error,
    Storage_BusType_Error,
    Storage_BusCfg_Malloc_Failed,
    Storage_ExtDevObj_Error,
    Storage_ModuleType_Error,
    Storage_ModuleInit_Error,
    Storage_ModuleAPI_Error,
    Storage_Read_Error,
    Storage_Write_Error,
    Storage_Erase_Error,
    Storage_NameMatched,
    Storage_SlotHeader_Error,
    Storage_ExternalFlash_NotAvailable,
    Storage_InternalFlash_NotAvailable,
    Storage_Class_Error,
    Storage_RW_Api_Error,
    Storage_No_Enough_Space,
    Storage_DataInfo_Error,
    Storage_DataSize_Overrange,
    Storage_BaseInfo_Updata_Error,
    Storage_DataAddr_Update_Error,
    Storage_FreeSlot_Update_Error,
    Storage_FreeSlot_Get_Error,
    Storage_FreeSlot_Addr_Error,
    Storage_ItemInfo_Error,
    Storage_CRC_Error,
    Storage_Update_DataSize_Error,
} Storage_ErrorCode_List;

typedef enum
{
    Internal_Flash = 0,
    External_Flash,
} Storage_MediumType_List;

typedef enum
{
    Para_Boot = 0,
    Para_Sys,
    Para_User,
} Storage_ParaClassType_List;

#pragma pack(1)
/* length must be 64Byte */
typedef struct
{
    uint8_t head_tag;
    uint8_t class;
    uint8_t name[STORAGE_NAME_LEN];
    uint32_t data_addr;
    uint16_t len;
    uint8_t reserved[12];
    uint16_t crc16;
    uint8_t end_tag;
} Storage_Item_TypeDef;

/* length must be 64Byte witchout payload data */
typedef struct
{
    uint32_t head_tag;
    uint8_t name[STORAGE_NAME_LEN];
    uint32_t total_data_size;
    uint32_t cur_slot_size;
    uint32_t nxt_addr;
    uint8_t align_size;
    /* storage data insert */
    /*
     * for example: storage 13 byte name as "data_1" then data slot should be like the diagram down below
     *  _________________________________________________________________________________________________________________________________________
     * |    head    |   Name  | total data size | cur slot | nxt addr | align size | storage data |   align    |       slot crc    |     end    |
     * | 0xEF0110EF |  data_1 |       16        |    16    |     0    |      3     | ............ |            |   comput crc by   | 0xFE1001FE |
     * |   4Byte    |  41Byte |      4Byte      |  4Byte   |   4Byte  |    1Byte   |     13Byte   |   3Byte    | current slot data |     4Byte  |
     * |____________|_________|_________________|__________|__________|____________|______________|____________|___________________|____________|
     *                                                                                     |______________|               ↑
     *                                                                                            |         16Byte        |
     *                                                                                            |______ comput crc _____|
     * 
     */
    uint16_t slot_crc;
    uint32_t end_tag;
} Storage_DataSlot_TypeDef;

typedef struct
{
    uint32_t head_tag;
    uint32_t total_size;
    uint32_t cur_slot_size;
    uint32_t nxt_addr;
    uint32_t end_tag;
} Storage_FreeSlot_TypeDef;

typedef struct
{
    uint32_t tab_addr;
    uint32_t data_sec_addr;
    uint32_t data_sec_size;
    uint32_t page_num;
    uint32_t tab_size;
    uint32_t free_slot_addr;
    uint32_t para_size;
    uint32_t para_num;
} Storage_BaseSecInfo_TypeDef;

/* software storage info */
typedef struct
{
    uint8_t tag[32];

    uint32_t base_addr;

    uint32_t total_size;
    uint32_t remain_size;
    uint32_t data_sec_size;

    Storage_BaseSecInfo_TypeDef boot_sec;
    Storage_BaseSecInfo_TypeDef sys_sec;
    Storage_BaseSecInfo_TypeDef user_sec;
} Storage_FlashInfo_TypeDef;
#pragma pack()

typedef union
{
    struct
    {
        uint8_t internal : 1;
        uint8_t res_1    : 3;

        uint8_t external : 1; 
        uint8_t res_2    : 3;
    } bit;

    uint8_t val;
} Storage_ModuleState_TypeDef;

/* hadware flash chip info */
typedef struct
{
    Storage_ExtFlash_BusType_List bus_type;
    Storage_ExtFlashChipType_List chip_type;

    uint32_t total_size;

    uint32_t page_num;
    uint32_t page_size;

    uint32_t bank_num;
    uint32_t bank_size;

    uint32_t block_num;
    uint32_t block_size;

    uint32_t sector_num;
    uint32_t sector_size;

    void *dev_obj;
    void *dev_api;
} Storage_ExtFLashDevObj_TypeDef;

typedef struct
{
    Storage_ModuleState_TypeDef module_enable_reg;
    Storage_ModuleState_TypeDef module_init_reg;

    uint8_t InternalFlash_Format_cnt;
    uint8_t ExternalFlash_Format_cnt;
    uint8_t ExternalFlash_ReInit_cnt;

    uint8_t InternalFlash_BuildTab_cnt;
    uint8_t ExternalFlash_BuildTab_cnt;
    
    uint8_t ExternalFlash_Error_Code;
    uint8_t ExternalFlash_Init_Error; /* use for trace the place where the error occur */

    void *ExtDev_ptr;       /* external flash chip device obj pointer */
    void *ExtBusCfg_Ptr;    /* external flash chip hardware bus config data pointer */

    bool init_state;
    uint8_t inuse;

    uint16_t module_prod_type;
    uint16_t module_prod_code;

    Storage_FlashInfo_TypeDef internal_info;
    Storage_FlashInfo_TypeDef external_info;
} Storage_Monitor_TypeDef;

typedef struct
{
    bool (*init)(Storage_ModuleState_TypeDef enable, Storage_ExtFLashDevObj_TypeDef *ExtDev);
    storage_handle (*search)(Storage_MediumType_List medium, Storage_ParaClassType_List class, const char *name);
    bool (*save)(storage_handle hdl, uint8_t *p_data, uint16_t size);
    bool (*get)(storage_handle hdl, uint8_t *p_data, uint16_t size);
    bool (*clear)(storage_handle hdl);
} Storage_TypeDef;

extern Storage_TypeDef Storage;

#endif
