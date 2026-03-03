/*
 * ES9018K2M.c
 *
 *  Created on: Sep 29, 2025
 *      Author: huang
 */

#include "es9018k2m.h"
#include "stdio.h" // 用于 printf 打印错误信息

// ===============================================
// 1. 核心方法实现
// ===============================================

/**
 * @brief 初始化 ES9018K2M 句柄。
 */
void ES9018K2M_InitHandle(ES9018K2M_Handle* handle, I2C_HandleTypeDef* hi2c, uint8_t Address)
{
    handle->hi2c = hi2c;
    handle->DeviceAddress = Address;
    printf("ES9018K2M Handle Initialized. Address: 0x%02X\n", handle->DeviceAddress);
}

/**
 * @brief 写入 ES9018K2M 寄存器
 */
HAL_StatusTypeDef ES9018K2M_WriteReg(ES9018K2M_Handle* handle, uint8_t RegAddr, uint8_t Data)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Write(
        handle->hi2c,
        handle->DeviceAddress,
        RegAddr,
        I2C_MEMADD_SIZE_8BIT,
        &Data,
        1,
        ES9018K2M_I2C_TIMEOUT
    );

    if (status != HAL_OK) {
        printf("ES9018K2M Write Error! Reg: 0x%02X, Data: 0x%02X, Status: %d\n", RegAddr, Data, status);
    }
    return status;
}

/**
 * @brief 读取 ES9018K2M 寄存器
 */
HAL_StatusTypeDef ES9018K2M_ReadReg(ES9018K2M_Handle* handle, uint8_t RegAddr, uint8_t* pData)
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Read(
        handle->hi2c,
        handle->DeviceAddress,
        RegAddr,
        I2C_MEMADD_SIZE_8BIT,
        pData,
        1,
        ES9018K2M_I2C_TIMEOUT
    );

    if (status != HAL_OK) {
        printf("ES9018K2M Read Error! Reg: 0x%02X, Status: %d\n", RegAddr, status);
    }
    return status;
}

// ===============================================
// 2. 控制方法实现
// ===============================================

/**
 * @brief 验证 ES9018K2M 芯片是否连接和工作正常
 */
HAL_StatusTypeDef ES9018K2M_CheckConnection(ES9018K2M_Handle* handle)
{
    uint8_t chip_status = 0;

    if (ES9018K2M_ReadReg(handle, ES9018K2M_REG_CHIP_STATUS, &chip_status) != HAL_OK)
    {
        printf("ES9018K2M Check Failed: I2C Communication Error.\n");
        return HAL_ERROR;
    }

    // 检查 Chip ID (D7:D4 位)，ES9018K2M ID 通常为 3 (0b0011)
    if ((chip_status >> 4) == 0x03)
    {
        printf("ES9018K2M Connection OK. Chip Status: 0x%02X\n", chip_status);
        return HAL_OK;
    }
    else
    {
        printf("ES9018K2M Check Failed. Unexpected Chip ID: 0x%02X\n", (chip_status >> 4));
        return HAL_ERROR;
    }
}

///**
// * @brief 对 ES9018K2M 进行基础配置 (I2S 模式, Soft Start等)
// */
//HAL_StatusTypeDef ES9018K2M_BasicConfig(ES9018K2M_Handle* handle)
//{
//    // 1. **Soft Start (软启动):** 启用 (Reg 0x06, D0=1)
//    if (ES9018K2M_WriteReg(handle, 0x06, 0x01) != HAL_OK) return HAL_ERROR;
//
//    // 2. **System Settings (Reg 0x00):** 默认 MCLK/LRCK 512，关闭 DSD
//    if (ES9018K2M_WriteReg(handle, ES9018K2M_REG_SYSTEM_SETTINGS, 0x02) != HAL_OK) return HAL_ERROR;
//
//    // 3. **Control Register (Reg 0x01):** 标准 I2S 24位格式 (0x00)
//    if (ES9018K2M_WriteReg(handle, ES9018K2M_REG_CONTROL, 0x00) != HAL_OK) return HAL_ERROR;
//
//    // 4. **Master Clock Divider (Reg 0x02):** PLL模式/同步模式
//    if (ES9018K2M_WriteReg(handle, 0x02, 0x02) != HAL_OK) return HAL_ERROR;
//
//    // 5. **Soft Start (软启动):** 禁用 (Reg 0x06, D0=0)
//    if (ES9018K2M_WriteReg(handle, 0x06, 0x00) != HAL_OK) return HAL_ERROR;
//
//    printf("ES9018K2M Basic Config Done.\n");
//    return HAL_OK;
//}

/**
 * @brief 执行 ES9018K2M 芯片的软复位操作。
 * @note 此函数会将 Reg 0x00 的 Bit 0 设置为 1 触发复位，然后设置回 0 恢复正常操作。
 * 同时保留了其他位（包括osc_drv和reserved位）的默认值。
 */
HAL_StatusTypeDef ES9018K2M_SoftReset(ES9018K2M_Handle* handle)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t reg0_value = 0;

    printf("ES9018K2M: Starting Soft Reset...\n");

    // 1. 读取当前 Reg 0x00 的值，以保留其他位的状态（尤其是osc_drv和reserved位）
    // 虽然文档说默认0x00，但为了通用性，先读再改是好习惯
    status = ES9018K2M_ReadReg(handle, ES9018K2M_REG_SYSTEM_SETTINGS, &reg0_value);
    if (status != HAL_OK) {
        printf("ES9018K2M Soft Reset Error: Failed to read Reg 0x00.\n");
        return status;
    }

    // 2. 将 Bit 0 (soft_reset) 设置为 1，其他位保持不变
    reg0_value |= (1 << 0); // 将 Bit 0 置 1
    status = ES9018K2M_WriteReg(handle, ES9018K2M_REG_SYSTEM_SETTINGS, reg0_value);
    if (status != HAL_OK) {
        printf("ES9018K2M Soft Reset Error: Failed to write Reg 0x00 for reset.\n");
        return status;
    }

    // 3. 短暂延时，让芯片有时间完成复位
    // 具体的复位时间通常在数据手册中给出，这里先用1ms作为示例
    HAL_Delay(1);

    // 4. 将 Bit 0 (soft_reset) 恢复为 0，以恢复正常操作
    reg0_value &= ~(1 << 0); // 将 Bit 0 置 0
    status = ES9018K2M_WriteReg(handle, ES9018K2M_REG_SYSTEM_SETTINGS, reg0_value);
    if (status != HAL_OK) {
        printf("ES9018K2M Soft Reset Error: Failed to write Reg 0x00 for normal operation.\n");
        return status;
    }

    printf("ES9018K2M: Soft Reset Completed.\n");
    return HAL_OK;
}

/**
 * @brief 配置 ES9018K2M 的输入设置 (寄存器 #1)。
 */
HAL_StatusTypeDef ES9018K2M_InputConfig(ES9018K2M_Handle* handle, const ES9018K2M_InputConfig_t* config)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t reg1_value = 0;

    // 组合所有配置位到 reg1_value
    // i2s_length (位 [7:6])
    reg1_value |= (config->i2s_length << 6);

    // i2s_mode (位 [5:4])
    reg1_value |= (config->i2s_mode << 4);

    // auto_input_select (位 [3:2])
    reg1_value |= (config->auto_input_select << 2);

    // input_select (位 [1:0])
    reg1_value |= config->input_select;

    // 写入寄存器 #1
    status = ES9018K2M_WriteReg(handle, 0x01, reg1_value);

    if (status == HAL_OK) {
        printf("ES9018K2M Input Config Done. Written Value: 0x%02X\n", reg1_value);
    } else {
        printf("ES9018K2M Input Config Error. Status: %d\n", status);
    }

    return status;
}

/**
 * @brief 配置 ES9018K2M 的通道映射设置 (寄存器 #11)。
 */
HAL_StatusTypeDef ES9018K2M_ChannelMapping(ES9018K2M_Handle* handle, const ES9018K2M_ChannelMapping_t* config)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t reg11_value = 0;

    // 默认值 Reg 0x11 = 0x02。
    // Bit 7 reserved: 0
    // Bit [6:4] spdif_sel = 0b000 (DATA_CLK)
    // Bit 3 ch2_analog_swap = 0b0 (normal)
    // Bit 2 ch1_analog_swap = 0b0 (normal)
    // Bit 1 ch2_sel = 0b1 (right)
    // Bit 0 ch1_sel = 0b0 (left)
    // 组合起来是 0b00000010 = 0x02

    // 1. 设置 reserved 位 [7] 为 0
    //    文档指出 Reserved Bits in Register #11 must be set to the indicated logic level to ensure correct device operation.
    //    由于默认值 0x02 中 Bit 7 是 0，所以我们保持为 0。

    // 2. 设置 spdif_sel (位 [6:4])
    reg11_value |= (config->spdif_sel << 4);

    // 3. 设置 ch2_analog_swap (位 [3])
    reg11_value |= (config->ch2_analog_swap << 3);

    // 4. 设置 ch1_analog_swap (位 [2])
    reg11_value |= (config->ch1_analog_swap << 2);

    // 5. 设置 ch2_sel (位 [1])
    reg11_value |= (config->ch2_sel << 1);

    // 6. 设置 ch1_sel (位 [0])
    reg11_value |= config->ch1_sel;

    // 写入寄存器 #11
    status = ES9018K2M_WriteReg(handle, 0x0B, reg11_value); // 0x0B 是寄存器 #11 的地址

    if (status == HAL_OK) {
        printf("ES9018K2M Channel Mapping Done. Written Value: 0x%02X\n", reg11_value);
    } else {
        printf("ES9018K2M Channel Mapping Error. Status: %d\n", status);
    }

    return status;
}

/**
 * @brief 设置电平衰减量
 */
HAL_StatusTypeDef ES9018K2M_SetAttenuation(ES9018K2M_Handle* handle, uint8_t Attenuation_half_dB)
{
    // Volume: 0x00 (Mute) to 0xFF (Max Volume)
    if (ES9018K2M_WriteReg(handle, ES9018K2M_REG_VOLUME_L, Attenuation_half_dB) != HAL_OK) return HAL_ERROR;
    if (ES9018K2M_WriteReg(handle, ES9018K2M_REG_VOLUME_R, Attenuation_half_dB) != HAL_OK) return HAL_ERROR;

    printf("ES9018K2M Attenuation Set to 0x%02X * 0.5dB.\n", Attenuation_half_dB);
    return HAL_OK;
}

/**
 * @brief 获取当前衰减值
 */
HAL_StatusTypeDef ES9018K2M_GetAttenuation(ES9018K2M_Handle* handle, uint8_t* pAttenuation_half_dB)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t volume_l = 0;
    uint8_t volume_r = 0;

    // 读取左右声道的衰减值
    status = ES9018K2M_ReadReg(handle, ES9018K2M_REG_VOLUME_L, &volume_l);
    if (status != HAL_OK) {
        printf("ES9018K2M Get Attenuation Error: Failed to read left channel.\n");
        return status;
    }

    status = ES9018K2M_ReadReg(handle, ES9018K2M_REG_VOLUME_R, &volume_r);
    if (status != HAL_OK) {
        printf("ES9018K2M Get Attenuation Error: Failed to read right channel.\n");
        return status;
    }

    // 验证左右声道衰减值是否一致（通常应该一致）
    if (volume_l != volume_r) {
        printf("ES9018K2M Warning: Left and right channel attenuation values differ (L: 0x%02X, R: 0x%02X)\n",
               volume_l, volume_r);
    }

    // 返回左声道的衰减值（或者可以返回平均值）
    *pAttenuation_half_dB = volume_l;

    printf("ES9018K2M Current Attenuation: 0x%02X * 0.5dB\n", volume_l);
    return HAL_OK;
}
