/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SESSION_GSL_H
#define SESSION_GSL_H

#define BUFF_FLAG_EOS 0x1

#include "gsl_intf.h"
#include "Session.h"
#include <dlfcn.h>
#include "apm_api.h"
#include "common_enc_dec_api.h"
#include "module_cmn_api.h"
#include "hw_intf_cmn_api.h"
#include "pcm_decoder_api.h"
#include "module_cmn_api.h"
#include "i2s_api.h"
#include "pcm_tdm_api.h"
#include "audio_dam_buffer_api.h"
#include "codec_dma_api.h"
#include "detection_cmn_api.h"

/* Param ID definitions */
#define PARAM_ID_MEDIA_FORMAT 0x0800100C
#define PARAM_ID_VOL_CTRL_MULTICHANNEL_GAIN 0x08001038
#define PARAM_ID_VOL_CTRL_MASTER_GAIN 0x08001035
#define PLAYBACK_VOLUME_MASTER_GAIN_DEFAULT 0x2000
#define PARAM_ID_DETECTION_ENGINE_CONFIG_VOICE_WAKEUP 0x08001049
#define PARAM_ID_VOICE_WAKEUP_BUFFERING_CONFIG 0x08001044

#define WSA_CODEC_DMA_CORE  LPAIF_WSA
#define VA_CODEC_DMA_CORE   LPAIF_VA
#define RXTX_CODEC_DMA_CORE LPAIF_RXTX

#define CODEC_RX0 1
#define CODEC_TX0 1
#define CODEC_RX1 2
#define CODEC_TX1 2
#define CODEC_RX2 3
#define CODEC_TX2 3
#define CODEC_RX3 4
#define CODEC_TX3 4
#define CODEC_RX4 5
#define CODEC_TX4 5
#define CODEC_RX5 6
#define CODEC_TX5 6
#define CODEC_RX6 7
#define CODEC_RX7 8


struct gslCmdGetReadWriteBufInfo {
    uint32_t buff_size;
    uint32_t num_buffs;
    uint32_t start_threshold;
    uint32_t stop_threshold;
    uint32_t attritubes;
};

struct __attribute__((__packed__)) volume_ctrl_channels_gain_config_t
{
    uint32_t channel_mask_lsb;
    uint32_t channel_mask_msb;
    uint32_t gain;
};

struct __attribute__((__packed__)) volume_ctrl_multichannel_gain_t
{
    uint32_t num_config;
    volume_ctrl_channels_gain_config_t gain_data[0];
};

struct audio_dam_downstream_setup_duration
{
    uint32_t num_output_ports;
    struct audio_dam_downstream_setup_duration_t port_cfgs[0];
};


class Stream;
class Session;

class SessionGsl : public Session
{
private:
    void * graphHandle;
    void * payload;
    size_t size = 0;
    size_t gkvLen, ckvLen, tkvLen;
    struct gslCmdGetReadWriteBufInfo *infoBuffer;
    static int seek;
    static void* gslLibHandle;
    int fileWrite(Stream *s, int tag, struct qal_buffer *buf, int * size, int flag);
    int fileRead(Stream *s, int tag, struct qal_buffer *buf, int * size);
    SessionGsl();
    
public:
    SessionGsl(std::shared_ptr<ResourceManager> Rm);
    ~SessionGsl();
    static int init(std::string acdbFile);
    static void deinit();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setConfig(Stream * s, configType type, int tag = 0) override;
    int setPayloadConfig(Stream *s);
    //int getConfig(Stream * s) override;
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int readBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int writeBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int read(Stream *s, int tag, struct qal_buffer *buf, int * size) override;
    int write(Stream *s, int tag, struct qal_buffer *buf, int * size, int flag) override;
    int setParameters(Stream *s, uint32_t param_id, void *payload) override;
    static void stCallBack(struct gsl_event_cb_params *event_params, void *client_data);
    struct gsl_key_vector *gkv;
    struct gsl_key_vector *ckv;
    struct gsl_key_vector *tkv;
};

#endif //SESSION_GSL_H