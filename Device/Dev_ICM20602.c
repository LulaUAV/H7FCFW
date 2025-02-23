#include "Dev_ICM20602.h"

#define ConvertToICM20602Trip_Reg(x) (x << 3)

/* internal function */
static bool DevICM20602_Regs_Read(DevICM20602Obj_TypeDef *Obj, uint32_t addr, uint8_t *tx, uint8_t *rx, uint16_t size);
static bool DevICM20602_Reg_Read(DevICM20602Obj_TypeDef *Obj, uint8_t addr, uint8_t *rx);
static bool DevICM20602_Reg_Write(DevICM20602Obj_TypeDef *Obj, uint8_t addr, uint8_t tx);

/* external function */
static bool DevICM20602_Detect(bus_trans_callback trans, cs_ctl_callback cs_ctl);
static bool DevICM20602_Config(DevICM20602Obj_TypeDef *Obj, 
                               ICM20602_SampleRate_List rate, 
                               ICM20602_GyrTrip_List GyrTrip, 
                               ICM20602_AccTrip_List AccTrip);
static void DevICM20602_PreInit(DevICM20602Obj_TypeDef *Obj,
                                cs_ctl_callback cs_ctl,
                                bus_trans_callback bus_trans,
                                delay_callback delay,
                                get_time_stamp_callback get_time_stamp);
static bool DevICM20602_Init(DevICM20602Obj_TypeDef *Obj);
static void DevICM20602_SetDRDY(DevICM20602Obj_TypeDef *Obj);
static bool DevICM20602_SwReset(DevICM20602Obj_TypeDef *Obj);
static bool DevICM20602_GetReady(DevICM20602Obj_TypeDef *Obj);
static bool DevICM20602_Sample(DevICM20602Obj_TypeDef *Obj);
static IMUData_TypeDef DevICM20602_Get_Data(DevICM20602Obj_TypeDef *Obj);
static IMU_Error_TypeDef DevICM20602_Get_InitError(DevICM20602Obj_TypeDef *Obj);
static IMUModuleScale_TypeDef DevICM20602_Get_Scale(const DevICM20602Obj_TypeDef *Obj);
static float DevICM20602_Get_Specified_AngularSpeed_Diff(const DevICM20602Obj_TypeDef *Obj);

/* external variable */
DevICM20602_TypeDef DevICM20602 = {
    .detect = DevICM20602_Detect,
    .config = DevICM20602_Config,
    .init = DevICM20602_Init,
    .pre_init = DevICM20602_PreInit,
    .reset = DevICM20602_SwReset,
    .set_ready = DevICM20602_SetDRDY,
    .get_ready = DevICM20602_GetReady,
    .sample = DevICM20602_Sample,
    .get_error = DevICM20602_Get_InitError,
    .get_data = DevICM20602_Get_Data,
    .get_scale = DevICM20602_Get_Scale,
    .get_gyr_angular_speed_diff = DevICM20602_Get_Specified_AngularSpeed_Diff,
};

static bool DevICM20602_Detect(bus_trans_callback trans, cs_ctl_callback cs_ctl)
{
    uint8_t Rx_Tmp[2] = {0};
    uint8_t Tx_Tmp[2] = {0};
    bool state = false;

    if (cs_ctl == NULL || trans == NULL)
        return false;

    Tx_Tmp[0] = ICM20602_WHO_AM_I | ICM20602_READ_MASK;

    /* cs low */
    cs_ctl(false);

    state = trans(Tx_Tmp, Rx_Tmp, 2);

    /* cs high */
    cs_ctl(true);

    if(!state)
        return false;

    if(Rx_Tmp[1] == ICM20602_DEV_ID)
        return true;

    return false;
}

static bool DevICM20602_Regs_Read(DevICM20602Obj_TypeDef *Obj, uint32_t addr, uint8_t *tx, uint8_t *rx, uint16_t size)
{
    bool state = false;
    uint8_t addr_tmp = addr | ICM20602_READ_MASK;
    uint8_t read_tmp = 0;

    if (Obj == NULL || Obj->cs_ctl == NULL || Obj->bus_trans == NULL)
        return false;

    /* CS Low */
    Obj->cs_ctl(false);

    state = Obj->bus_trans(&addr_tmp, &read_tmp, 1);
    state = Obj->bus_trans(tx, rx, size);

    /* CS High */
    Obj->cs_ctl(true);

    return state;
}

static bool DevICM20602_Reg_Read(DevICM20602Obj_TypeDef *Obj, uint8_t addr, uint8_t *rx)
{
    uint8_t Rx_Tmp[2] = {0};
    uint8_t Tx_Tmp[2] = {0};
    bool state = false;

    if (Obj == NULL || Obj->cs_ctl == NULL || Obj->bus_trans == NULL)
        return false;

    Tx_Tmp[0] = addr | ICM20602_READ_MASK;

    /* cs low */
    Obj->cs_ctl(false);

    state = Obj->bus_trans(Tx_Tmp, Rx_Tmp, 2);

    /* cs high */
    Obj->cs_ctl(true);

    *rx = Rx_Tmp[1];

    return state;
}

static bool DevICM20602_Reg_Write(DevICM20602Obj_TypeDef *Obj, uint8_t addr, uint8_t tx)
{
    uint8_t Rx_Tmp[2] = {0};
    uint8_t Tx_Tmp[2] = {0};
    bool state = false;

    if (Obj == NULL || Obj->cs_ctl == NULL || Obj->bus_trans == NULL)
        return false;

    Tx_Tmp[0] = addr;
    Tx_Tmp[1] = tx;

    /* cs low */
    Obj->cs_ctl(false);

    state = Obj->bus_trans(Tx_Tmp, Rx_Tmp, 2);

    /* cs high */
    Obj->cs_ctl(true);

    return state;
}

static bool DevICM20602_Config(DevICM20602Obj_TypeDef *Obj, 
                               ICM20602_SampleRate_List rate, 
                               ICM20602_GyrTrip_List GyrTrip, 
                               ICM20602_AccTrip_List AccTrip)
{
    if(Obj == NULL)
        return false;

    switch ((uint8_t)Obj->rate)
    {
    case ICM20602_SampleRate_8K:
    case ICM20602_SampleRate_4K:
    case ICM20602_SampleRate_2K:
    case ICM20602_SampleRate_1K:
        break;

    default:
        return false;
    }

    Obj->rate = rate;

    switch ((uint8_t)AccTrip)
    {
    case ICM20602_Acc_2G:
        Obj->acc_scale = ICM20602_ACC_2G_SCALE;
        Obj->PHY_AccTrip_Val = 2;
        break;

    case ICM20602_Acc_4G:
        Obj->acc_scale = ICM20602_ACC_4G_SCALE;
        Obj->PHY_AccTrip_Val = 4;
        break;

    case ICM20602_Acc_8G:
        Obj->acc_scale = ICM20602_ACC_8G_SCALE;
        Obj->PHY_AccTrip_Val = 8;
        break;

    case ICM20602_Acc_16G:
        Obj->acc_scale = ICM20602_ACC_16G_SCALE;
        Obj->PHY_AccTrip_Val = 16;
        break;

    default:
        return false;
    }

    switch ((uint8_t)GyrTrip)
    {
    case ICM20602_Gyr_250DPS:
        Obj->gyr_scale = ICM20602_GYR_250DPS_SCALE;
        Obj->PHY_GyrTrip_Val = 250;
        break;

    case ICM20602_Gyr_500DPS:
        Obj->gyr_scale = ICM20602_GYR_500DPS_SCALE;
        Obj->PHY_GyrTrip_Val = 500;
        break;

    case ICM20602_Gyr_1000DPS:
        Obj->gyr_scale = ICM20602_GYR_1000DPS_SCALE;
        Obj->PHY_GyrTrip_Val = 1000;
        break;

    case ICM20602_Gyr_2000DPS:
        Obj->gyr_scale = ICM20602_GYR_2000DPS_SCALE;
        Obj->PHY_GyrTrip_Val = 2000;
        break;

    default:
        return false;
    }

    // set Trip Reg val
    Obj->AccTrip = ConvertToICM20602Trip_Reg(AccTrip);
    Obj->GyrTrip = ConvertToICM20602Trip_Reg(GyrTrip);

    return true;
}

static void DevICM20602_PreInit(DevICM20602Obj_TypeDef *Obj,
                                cs_ctl_callback cs_ctl,
                                bus_trans_callback bus_trans,
                                delay_callback delay,
                                get_time_stamp_callback get_time_stamp)
{
    Obj->cs_ctl = cs_ctl;
    Obj->bus_trans = bus_trans;
    Obj->delay = delay;
    Obj->get_timestamp = get_time_stamp;
}

static bool DevICM20602_Init(DevICM20602Obj_TypeDef *Obj)
{
    uint8_t read_out = 0;

    if ((Obj == NULL) ||
        (Obj->bus_trans == NULL) ||
        (Obj->cs_ctl == NULL))
    {
        IMUData_SetError(&(Obj->error), ICM20602_Obj_Error, __FUNCTION__, __LINE__, 0, 0, 0);
        return false;
    }

    /* reset device */
    if (!DevICM20602_SwReset(Obj))
    {
        IMUData_SetError(&(Obj->error), ICM20602_Reset_Error, __FUNCTION__, __LINE__, 0, 0, 0);
        return false;
    }

    DevICM20602_Reg_Read(Obj, ICM20602_WHO_AM_I, &read_out);
    Obj->delay(10);

    if (read_out != ICM20602_DEV_ID)
    {
        IMUData_SetError(&(Obj->error), ICM20602_DevID_Error, __FUNCTION__, __LINE__, ICM20602_WHO_AM_I, read_out, ICM20602_DEV_ID);
        return false;
    }

    /* set oscillator clock */
    DevICM20602_Reg_Write(Obj, ICM20602_PWR_MGMT_1, 0x01);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_PWR_MGMT_1, &read_out);
    if (read_out != 0x01)
    {
        IMUData_SetError(&(Obj->error), ICM20602_OSC_Error, __FUNCTION__, __LINE__, ICM20602_PWR_MGMT_1, read_out, 0x01);
        return false;
    }

    /* enable gyro and acc */
    DevICM20602_Reg_Write(Obj, ICM20602_PWR_MGMT_2, 0x00);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_PWR_MGMT_2, &read_out);
    if (read_out != 0x00)
    {
        IMUData_SetError(&(Obj->error), ICM20602_InertialEnable_Error, __FUNCTION__, __LINE__, ICM20602_PWR_MGMT_2, read_out, 0x00);
        return false;
    }

    /* disable iic interface */
    DevICM20602_Reg_Write(Obj, ICM20602_I2C_IF, 0x40);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_I2C_IF, &read_out);
    if (read_out != 0x40)
    {
        IMUData_SetError(&(Obj->error), ICM20602_InterfaceSet_Error, __FUNCTION__, __LINE__, ICM20602_I2C_IF, read_out, 0x40);
        return false;
    }

    /* set sample rate */
    DevICM20602_Reg_Write(Obj, ICM20602_SMPLRT_DIV, Obj->rate);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_SMPLRT_DIV, &read_out);
    if (read_out != Obj->rate)
    {
        IMUData_SetError(&(Obj->error), ICM20602_RateSet_Error, __FUNCTION__, __LINE__, ICM20602_SMPLRT_DIV, read_out, Obj->rate);
        return false;
    }

    /* set acc trip */
    DevICM20602_Reg_Write(Obj, ICM20602_ACCEL_CONFIG, Obj->AccTrip);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_ACCEL_CONFIG, &read_out);
    if (read_out != Obj->AccTrip)
    {
        IMUData_SetError(&(Obj->error), ICM20602_AccSet_Error, __FUNCTION__, __LINE__, ICM20602_ACCEL_CONFIG, read_out, Obj->AccTrip);
        return false;
    }

    /* set gyr trip */
    DevICM20602_Reg_Write(Obj, ICM20602_GYRO_CONFIG, Obj->GyrTrip);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_GYRO_CONFIG, &read_out);
    if (read_out != Obj->GyrTrip)
    {
        IMUData_SetError(&(Obj->error), ICM20602_GyrSet_Error, __FUNCTION__, __LINE__, ICM20602_GYRO_CONFIG, read_out, Obj->GyrTrip);
        return false;
    }

    /* enable odr int output */
    DevICM20602_Reg_Write(Obj, ICM20602_INT_PIN_CFG, 0x60);
    Obj->delay(10);

    DevICM20602_Reg_Read(Obj, ICM20602_INT_PIN_CFG, &read_out);
    if (read_out != 0x60)
    {
        IMUData_SetError(&(Obj->error), ICM20602_IntPinSet_Error, __FUNCTION__, __LINE__, ICM20602_INT_PIN_CFG, read_out, 0x60);
        return false;
    }

    IMUData_SetError(&(Obj->error), ICM20602_No_Error, "", 0, 0, 0, 0);
    return true;
}

static void DevICM20602_SetDRDY(DevICM20602Obj_TypeDef *Obj)
{
    if(Obj)
        Obj->drdy = true;
}

static bool DevICM20602_SwReset(DevICM20602Obj_TypeDef *Obj)
{
    uint8_t read_out = 0;
    uint32_t reset_UsRt = 0;

    DevICM20602_Reg_Write(Obj, ICM20602_PWR_MGMT_1, ICM20602_RESET_CMD);
    Obj->delay(20);
    reset_UsRt = Obj->get_timestamp();

    // do
    // {
    //     if ((Obj->get_timestamp() - reset_UsRt) >= ICM20602_RESET_TIMEOUT)
    //         return false;

    //     DevICM20602_Reg_Read(Obj, ICM20602_PWR_MGMT_1, &read_out);
    // } while (ICM20602_RESET_SUCCESS != read_out);

    return true;
}

static bool DevICM20602_GetReady(DevICM20602Obj_TypeDef *Obj)
{
    if(Obj == NULL)
        return false;
        
    return Obj->drdy;
}

static bool DevICM20602_Sample(DevICM20602Obj_TypeDef *Obj)
{
    if (Obj == NULL)
        return false;

    return true;
}

static IMUData_TypeDef DevICM20602_Get_Data(DevICM20602Obj_TypeDef *Obj)
{
    IMUData_TypeDef tmp;
    memset(&tmp, NULL, sizeof(tmp));

    if ((Obj->error.code == ICM20602_No_Error) && !Obj->update)
    {
        Obj->drdy = false;
        return Obj->OriData;
    }

    return tmp;
}

static IMU_Error_TypeDef DevICM20602_Get_InitError(DevICM20602Obj_TypeDef *Obj)
{
    return Obj->error;
}

static IMUModuleScale_TypeDef DevICM20602_Get_Scale(const DevICM20602Obj_TypeDef *Obj)
{
    IMUModuleScale_TypeDef scale;

    scale.acc_scale = Obj->acc_scale;
    scale.gyr_scale = Obj->gyr_scale;

    return scale;
}

/* get specified angular speed diff per sec */
static float DevICM20602_Get_Specified_AngularSpeed_Diff(const DevICM20602Obj_TypeDef *Obj)
{
    return Obj->PHY_GyrTrip_Val / 1000.0f;
}