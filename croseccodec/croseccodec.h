#ifndef __CROS_EC_REGS_H__
#define __CROS_EC_REGS_H__

#define BIT(nr) (1UL << (nr))

/* Commands for audio codec. */
#define EC_CMD_EC_CODEC 0x00BC

enum ec_codec_subcmd {
	EC_CODEC_GET_CAPABILITIES = 0x0,
	EC_CODEC_GET_SHM_ADDR = 0x1,
	EC_CODEC_SET_SHM_ADDR = 0x2,
	EC_CODEC_SUBCMD_COUNT,
};

enum ec_codec_cap {
	EC_CODEC_CAP_WOV_AUDIO_SHM = 0,
	EC_CODEC_CAP_WOV_LANG_SHM = 1,
	EC_CODEC_CAP_LAST = 32,
};

enum ec_codec_shm_id {
	EC_CODEC_SHM_ID_WOV_AUDIO = 0x0,
	EC_CODEC_SHM_ID_WOV_LANG = 0x1,
	EC_CODEC_SHM_ID_LAST,
};

enum ec_codec_shm_type {
	EC_CODEC_SHM_TYPE_EC_RAM = 0x0,
	EC_CODEC_SHM_TYPE_SYSTEM_RAM = 0x1,
};

#include <pshpack1.h>

struct ec_param_ec_codec_get_shm_addr {
	UINT8 shm_id;
	UINT8 reserved[3];
};

#include <poppack.h>

#include <pshpack4.h>

struct ec_param_ec_codec_set_shm_addr {
	UINT64 phys_addr;
	UINT32 len;
	UINT8 shm_id;
	UINT8 reserved[3];
};

struct ec_param_ec_codec {
	UINT8 cmd; /* enum ec_codec_subcmd */
	UINT8 reserved[3];

	union {
		struct ec_param_ec_codec_get_shm_addr
			get_shm_addr_param;
		struct ec_param_ec_codec_set_shm_addr
			set_shm_addr_param;
	};
};

struct ec_response_ec_codec_get_capabilities {
	UINT32 capabilities;
};

struct ec_response_ec_codec_get_shm_addr {
	UINT64 phys_addr;
	UINT32 len;
	UINT8 type;
	UINT8 reserved[3];
};

#include <poppack.h>

/*****************************************************************************/

/* Commands for DMIC on audio codec. */
#define EC_CMD_EC_CODEC_DMIC 0x00BD

enum ec_codec_dmic_subcmd {
	EC_CODEC_DMIC_GET_MAX_GAIN = 0x0,
	EC_CODEC_DMIC_SET_GAIN_IDX = 0x1,
	EC_CODEC_DMIC_GET_GAIN_IDX = 0x2,
	EC_CODEC_DMIC_SUBCMD_COUNT,
};

enum ec_codec_dmic_channel {
	EC_CODEC_DMIC_CHANNEL_0 = 0x0,
	EC_CODEC_DMIC_CHANNEL_1 = 0x1,
	EC_CODEC_DMIC_CHANNEL_2 = 0x2,
	EC_CODEC_DMIC_CHANNEL_3 = 0x3,
	EC_CODEC_DMIC_CHANNEL_4 = 0x4,
	EC_CODEC_DMIC_CHANNEL_5 = 0x5,
	EC_CODEC_DMIC_CHANNEL_6 = 0x6,
	EC_CODEC_DMIC_CHANNEL_7 = 0x7,
	EC_CODEC_DMIC_CHANNEL_COUNT,
};

#include <pshpack1.h>

struct ec_param_ec_codec_dmic_set_gain_idx {
	UINT8 channel; /* enum ec_codec_dmic_channel */
	UINT8 gain;
	UINT8 reserved[2];
};

struct ec_param_ec_codec_dmic_get_gain_idx {
	UINT8 channel; /* enum ec_codec_dmic_channel */
	UINT8 reserved[3];
};

#include <poppack.h>

#include <pshpack4.h>

struct ec_param_ec_codec_dmic {
	UINT8 cmd; /* enum ec_codec_dmic_subcmd */
	UINT8 reserved[3];

	union {
		struct ec_param_ec_codec_dmic_set_gain_idx
			set_gain_idx_param;
		struct ec_param_ec_codec_dmic_get_gain_idx
			get_gain_idx_param;
	};
};

#include <poppack.h>

#include <pshpack1.h>

struct ec_response_ec_codec_dmic_get_max_gain {
	UINT8 max_gain;
};

struct ec_response_ec_codec_dmic_get_gain_idx {
	UINT8 gain;
};

#include <poppack.h>

/*****************************************************************************/

/* Commands for I2S RX on audio codec. */

#define EC_CMD_EC_CODEC_I2S_RX 0x00BE

enum ec_codec_i2s_rx_subcmd {
	EC_CODEC_I2S_RX_ENABLE = 0x0,
	EC_CODEC_I2S_RX_DISABLE = 0x1,
	EC_CODEC_I2S_RX_SET_SAMPLE_DEPTH = 0x2,
	EC_CODEC_I2S_RX_SET_DAIFMT = 0x3,
	EC_CODEC_I2S_RX_SET_BCLK = 0x4,
	EC_CODEC_I2S_RX_RESET = 0x5,
	EC_CODEC_I2S_RX_SUBCMD_COUNT,
};

enum ec_codec_i2s_rx_sample_depth {
	EC_CODEC_I2S_RX_SAMPLE_DEPTH_16 = 0x0,
	EC_CODEC_I2S_RX_SAMPLE_DEPTH_24 = 0x1,
	EC_CODEC_I2S_RX_SAMPLE_DEPTH_COUNT,
};

enum ec_codec_i2s_rx_daifmt {
	EC_CODEC_I2S_RX_DAIFMT_I2S = 0x0,
	EC_CODEC_I2S_RX_DAIFMT_RIGHT_J = 0x1,
	EC_CODEC_I2S_RX_DAIFMT_LEFT_J = 0x2,
	EC_CODEC_I2S_RX_DAIFMT_COUNT,
};

#include <pshpack1.h>

struct ec_param_ec_codec_i2s_rx_set_sample_depth {
	UINT8 depth;
	UINT8 reserved[3];
};

struct ec_param_ec_codec_i2s_rx_set_gain {
	UINT8 left;
	UINT8 right;
	UINT8 reserved[2];
};

struct ec_param_ec_codec_i2s_rx_set_daifmt {
	UINT8 daifmt;
	UINT8 reserved[3];
};

#include <poppack.h>

#include <pshpack4.h>

struct ec_param_ec_codec_i2s_rx_set_bclk {
	UINT32 bclk;
};

struct ec_param_ec_codec_i2s_rx {
	UINT8 cmd; /* enum ec_codec_i2s_rx_subcmd */
	UINT8 reserved[3];

	union {
		struct ec_param_ec_codec_i2s_rx_set_sample_depth
			set_sample_depth_param;
		struct ec_param_ec_codec_i2s_rx_set_daifmt
			set_daifmt_param;
		struct ec_param_ec_codec_i2s_rx_set_bclk
			set_bclk_param;
	};
};

#include <poppack.h>

/*****************************************************************************/
/* Commands for WoV on audio codec. */

#define EC_CMD_EC_CODEC_WOV 0x00BF

enum ec_codec_wov_subcmd {
	EC_CODEC_WOV_SET_LANG = 0x0,
	EC_CODEC_WOV_SET_LANG_SHM = 0x1,
	EC_CODEC_WOV_GET_LANG = 0x2,
	EC_CODEC_WOV_ENABLE = 0x3,
	EC_CODEC_WOV_DISABLE = 0x4,
	EC_CODEC_WOV_READ_AUDIO = 0x5,
	EC_CODEC_WOV_READ_AUDIO_SHM = 0x6,
	EC_CODEC_WOV_SUBCMD_COUNT,
};

#include <pshpack4.h>

/*
 * @hash is SHA256 of the whole language model.
 * @total_len indicates the length of whole language model.
 * @offset is the cursor from the beginning of the model.
 * @buf is the packet buffer.
 * @len denotes how many bytes in the buf.
 */
struct ec_param_ec_codec_wov_set_lang {
	UINT8 hash[32];
	UINT32 total_len;
	UINT32 offset;
	UINT8 buf[128];
	UINT32 len;
};

struct ec_param_ec_codec_wov_set_lang_shm {
	UINT8 hash[32];
	UINT32 total_len;
};

struct ec_param_ec_codec_wov {
	UINT8 cmd; /* enum ec_codec_wov_subcmd */
	UINT8 reserved[3];

	union {
		struct ec_param_ec_codec_wov_set_lang
			set_lang_param;
		struct ec_param_ec_codec_wov_set_lang_shm
			set_lang_shm_param;
	};
};

struct ec_response_ec_codec_wov_get_lang {
	UINT8 hash[32];
};

struct ec_response_ec_codec_wov_read_audio {
	UINT8 buf[128];
	UINT32 len;
};

struct ec_response_ec_codec_wov_read_audio_shm {
	UINT32 offset;
	UINT32 len;
};

#include <poppack.h>

/**
 * struct cros_ec_command - Information about a ChromeOS EC command.
 * @version: Command version number (often 0).
 * @command: Command to send (EC_CMD_...).
 * @outsize: Outgoing length in bytes.
 * @insize: Max number of bytes to accept from the EC.
 * @result: EC's response to the command (separate from communication failure).
 * @data: Where to put the incoming data from EC and outgoing data to EC.
 */
struct cros_ec_command {
    UINT32 version;
    UINT32 command;
    UINT32 outsize;
    UINT32 insize;
    UINT32 result;
    UINT8 data[];
};

#endif /* __CROS_EC_REGS_H__ */