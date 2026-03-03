/*
 * ES9018K2M.h
 *
 *  Created on: Sep 29, 2025
 *      Author: huang
 */

#ifndef INC_ES9018K2M_H_
#define INC_ES9018K2M_H_

#include "main.h"

// ===============================================
// ES9018K2M constants definition
// ===============================================
// R&W Registers
#define ES9018K2M_REG_SYSTEM_SETTINGS   	0x00	// Register #0: System Settings
#define ES9018K2M_REG_INPUT_CONFIG			0x01	// Register #1: Input Configuration
#define ES9018K2M_REG_SOFT_VOL_CON1			0x04	// Register #4: Soft Volume Control 1 (Automute Time)
#define ES9018K2M_REG_SOFT_VOL_CON2			0x05	// Register #5: Soft Volume Control 2 (Automute Level)
#define ES9018K2M_REG_SOFT_VOL_CON3			0x06	// Register #6: Soft Volume Control 3 and De-emphasis
#define ES9018K2M_REG_GENERAL_SETTINGS		0x07	// Register #7: General Settings
#define ES9018K2M_REG_GPIO_CONFIG			0x08	// Register #8: GPIO Configuration
#define ES9018K2M_REG_MASTER_MODE_CON		0x0A	// Register #10: Master Mode Control
#define ES9018K2M_REG_CH_MAPPING			0x0B	// Register #11: Channel Mapping
#define ES9018K2M_REG_DPLL_ASRC				0x0C	// Register #12: DPLL/ASRC Settings
#define ES9018K2M_REG_THD_COMPEN			0x0D	// Register #13: THD Compensation
#define ES9018K2M_REG_SOFT_START_SET		0x0E	// Register #14: Soft Start Settings
#define ES9018K2M_REG_VOLUME_L          	0x0F	// Register #15: Volume 1
#define ES9018K2M_REG_VOLUME_R          	0x10	// Register #16: Volume 2
#define ES9018K2M_REG_MASTER_TRIM_REG17  	0x11	// Register #17: Master Trim (LSB)
#define ES9018K2M_REG_MASTER_TRIM_REG18  	0x12	// Register #18: Master Trim
#define ES9018K2M_REG_MASTER_TRIM_REG19  	0x13	// Register #19: Master Trim
#define ES9018K2M_REG_MASTER_TRIM_REG20  	0x14	// Register #20: Master Trim (MSB)
#define ES9018K2M_REG_GPIO_INPUT_SEL_OSF	0x15	// Register #21: GPIO Input Selection and OSF Bypass
#define ES9018K2M_REG_2HAR_COMPEN_LSB		0x16	// Register #22: 2nd Harmonic Compensation Coefficients (LSB)
#define ES9018K2M_REG_2HAR_COMPEN_MSB		0x17	// Register #23: 2nd Harmonic Compensation Coefficients (MSB)
#define ES9018K2M_REG_3HAR_COMPEN_LSB		0x18	// Register #24: 3nd Harmonic Compensation Coefficients (LSB)
#define ES9018K2M_REG_3HAR_COMPEN_MSB		0x19	// Register #25: 3nd Harmonic Compensation Coefficients (MSB)
#define ES9018K2M_REG_PG_FLT_ADDR			0x1A	// Register #26: Programmable Filter Address
#define ES9018K2M_REG_PG_FLT_COEFF_REG27	0x1B	// Register #27: Programmable Filter Coefficient (LSB)
#define ES9018K2M_REG_PG_FLT_COEFF_REG28	0x1C	// Register #28: Programmable Filter Coefficient
#define ES9018K2M_REG_PG_FLT_COEFF_REG29	0x1D	// Register #29: Programmable Filter Coefficient (MSB)
#define ES9018K2M_REG_PG_FLT_CON			0x1E	// Register #30: Programmable Filter Control
// Read-Only Registers
#define ES9018K2M_REG_CHIP_STATUS       	0x40	// Register #64: Chip Status
#define ES9018K2M_REG_GPIO_STATUS			0x41	// Register #65: GPIO Status
#define ES9018K2M_REG_DPLL_RATIO_REG66		0x42	// Register #66: DPLL Ratio (LSB)
#define ES9018K2M_REG_DPLL_RATIO_REG67		0x43	// Register #67: DPLL Ratio
#define ES9018K2M_REG_DPLL_RATIO_REG68		0x44	// Register #68: DPLL Ratio
#define ES9018K2M_REG_DPLL_RATIO_REG69		0x45	// Register #69: DPLL Ratio (MSB)
#define ES9018K2M_REG_CH_STATUS_REG70(x)	(0x46+x)// Register #93-70: Channel Status
// Default I2C timeout (ms)
#define ES9018K2M_I2C_TIMEOUT           100

// ===============================================
// Control structure definition
// ===============================================

/**
 * @brief ES9018K2M DAC Control structure (Handle)
 */
typedef struct {
    /** @brief I2C 8位从设备地址 (已左移一位, 如 0x90 或 0x92) */
    uint8_t DeviceAddress;

    /** @brief I2C 硬件句柄指针 */
    I2C_HandleTypeDef* hi2c;

} ES9018K2M_Handle;

// i2s_length (位 [7:6])
typedef enum {
    ES9018K2M_I2S_LENGTH_16BIT = 0b00,
    ES9018K2M_I2S_LENGTH_24BIT = 0b01,
    ES9018K2M_I2S_LENGTH_32BIT = 0b10 // 默认值，0b11 也是 32bit
} ES9018K2M_I2S_Length;

// i2s_mode (位 [5:4])
typedef enum {
    ES9018K2M_I2S_MODE_I2S = 0b00, // 默认值
    ES9018K2M_I2S_MODE_LJ  = 0b01, // Left Justified
    // 0b10 和 0b11 也是 I2S 和 LJ mode，根据文档描述，为了避免混淆，这里只列出最常用的。
} ES9018K2M_I2S_Mode;

// auto_input_select (位 [3:2])
typedef enum {
    ES9018K2M_AUTO_INPUT_SELECT_DISABLED      = 0b00, // 只使用 input_select
    ES9018K2M_AUTO_INPUT_SELECT_I2S_DSD       = 0b01,
    ES9018K2M_AUTO_INPUT_SELECT_I2S_SPDIF     = 0b10,
    ES9018K2M_AUTO_INPUT_SELECT_I2S_SPDIF_DSD = 0b11  // 默认值
} ES9018K2M_AutoInputSelect;

// input_select (位 [1:0])
typedef enum {
    ES9018K2M_INPUT_SELECT_I2S   = 0b00, // 默认值
    ES9018K2M_INPUT_SELECT_SPDIF = 0b01,
    ES9018K2M_INPUT_SELECT_DSD   = 0b11,
    // 0b10 是 reserved
} ES9018K2M_InputSelect;

/**
 * @brief ES9018K2M 输入配置参数结构体
 */
typedef struct {
    ES9018K2M_I2S_Length      i2s_length;
    ES9018K2M_I2S_Mode        i2s_mode;
    ES9018K2M_AutoInputSelect auto_input_select;
    ES9018K2M_InputSelect     input_select;
} ES9018K2M_InputConfig_t;

// spdif_sel (位 [6:4])
typedef enum {
    ES9018K2M_SPDIF_SEL_DATA_CLK = 0b000, // 默认值
    ES9018K2M_SPDIF_SEL_DATA2    = 0b001,
    ES9018K2M_SPDIF_SEL_DATA1    = 0b010,
    ES9018K2M_SPDIF_SEL_GPIO1    = 0b011,
    ES9018K2M_SPDIF_SEL_GPIO2    = 0b100,
    // 0b101 - 0b111 为 reserved
} ES9018K2M_SPDIF_Select;

// chX_analog_swap (位 [3] 和 [2])
typedef enum {
    ES9018K2M_ANALOG_SWAP_NORMAL = 0, // 默认值
    ES9018K2M_ANALOG_SWAP_DAC_B  = 1  // Swap DAC A and DAC B
} ES9018K2M_AnalogSwap;

// chX_sel (位 [1] 和 [0])
typedef enum {
    ES9018K2M_CHANNEL_SEL_LEFT  = 0, // ch1_sel 默认 Left
    ES9018K2M_CHANNEL_SEL_RIGHT = 1  // ch2_sel 默认 Right
} ES9018K2M_ChannelSelect;

/**
 * @brief ES9018K2M 通道映射配置参数结构体
 */
typedef struct {
    ES9018K2M_SPDIF_Select    spdif_sel;         // SPDIF 数据源选择
    ES9018K2M_AnalogSwap      ch2_analog_swap;   // 通道2模拟输出交换
    ES9018K2M_AnalogSwap      ch1_analog_swap;   // 通道1模拟输出交换
    ES9018K2M_ChannelSelect   ch2_sel;           // 通道2数字输入选择 (Left/Right)
    ES9018K2M_ChannelSelect   ch1_sel;           // 通道1数字输入选择 (Left/Right)
} ES9018K2M_ChannelMapping_t;

// 使用 extern "C" 块封装所有的 C 函数声明
#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 3. 公共方法声明
// ===============================================

/**
 * @brief 初始化 ES9018K2M 句柄。
 * @param handle ES9018K2M 句柄实例指针
 * @param hi2c 传入 I2C 句柄指针 (例如 &hi2c1)
 * @param Address 传入 I2C 8位地址 (例如 0x90 或 0x92)
 */
void ES9018K2M_InitHandle(ES9018K2M_Handle* handle, I2C_HandleTypeDef* hi2c, uint8_t Address);

/**
 * @brief 写入 ES9018K2M 寄存器
 * @param handle ES9018K2M 句柄实例指针
 * @param RegAddr 寄存器地址 (8位)
 * @param Data 要写入的数据
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_WriteReg(ES9018K2M_Handle* handle, uint8_t RegAddr, uint8_t Data);

/**
 * @brief 读取 ES9018K2M 寄存器
 * @param handle ES9018K2M 句柄实例指针
 * @param RegAddr 寄存器地址 (8位)
 * @param pData 用于存储读取数据的指针
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_ReadReg(ES9018K2M_Handle* handle, uint8_t RegAddr, uint8_t* pData);


/**
 * @brief 验证 ES9018K2M 芯片是否连接和工作正常 (读取 Chip Status)
 * @param handle ES9018K2M 句柄实例指针
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_CheckConnection(ES9018K2M_Handle* handle);

///**
// * @brief 对 ES9018K2M 进行基础配置 (如 I2S 模式，建议在CheckConnection成功后调用)
// * @param handle ES9018K2M 句柄实例指针
// * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
// */
//HAL_StatusTypeDef ES9018K2M_BasicConfig(ES9018K2M_Handle* handle);

/**
 * @brief 执行 ES9018K2M 芯片的软复位操作。
 * @param handle ES9018K2M 句柄实例指针
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_SoftReset(ES9018K2M_Handle* handle);

/**
 * @brief 配置 ES9018K2M 的输入设置 (寄存器 #1)。
 * @param handle ES9018K2M 句柄实例指针
 * @param config 包含所有输入配置参数的结构体指针
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_InputConfig(ES9018K2M_Handle* handle, const ES9018K2M_InputConfig_t* config);

/**
 * @brief 配置 ES9018K2M 的通道映射设置 (寄存器 #11)。
 * @param handle ES9018K2M 句柄实例指针
 * @param config 包含所有通道映射参数的结构体指针
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_ChannelMapping(ES9018K2M_Handle* handle, const ES9018K2M_ChannelMapping_t* config);

/**
 * @brief 设置电平衰减量
 * @param handle ES9018K2M 句柄实例指针
 * @param Attenuation_half_dB 衰减值，步进0.5dB (0x00: 无衰减, 0xFF: 衰减最大)
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_SetAttenuation(ES9018K2M_Handle* handle, uint8_t Attenuation_half_dB);

/**
 * @brief 获取当前衰减值
 * @param handle ES9018K2M 句柄实例指针
 * @param pAttenuation_half_dB 指向存储衰减值的指针 (0x00: 无衰减, 0xFF: 衰减最大)
 * @return HAL 状态 (HAL_OK 或 HAL_ERROR)
 */
HAL_StatusTypeDef ES9018K2M_GetAttenuation(ES9018K2M_Handle* handle, uint8_t* pAttenuation_half_dB);

#ifdef __cplusplus
}
#endif

#endif /* INC_ES9018K2M_H_ */
