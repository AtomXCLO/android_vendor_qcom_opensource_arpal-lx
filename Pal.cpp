/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "PAL: API"

#include <unistd.h>
#include <stdlib.h>
#include <PalApi.h>
#include "Stream.h"
#include "ResourceManager.h"
#include "PalCommon.h"
class Stream;

/*
 * enable_gcov - Enable gcov for pal
 *
 * Prerequisites
 *   Should be call from CATF
 */

/*void enable_gcov()
{
    __gcov_flush();
}*/

static void notify_concurrent_stream(pal_stream_type_t type,
                                     pal_stream_direction_t dir,
                                     bool active)
{
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    if (!rm) {
        PAL_ERR(LOG_TAG,"Resource manager unavailable");
        return;
    }

    PAL_DBG(LOG_TAG, "Notify concurrent stream type %d, direction %d, active %d",
        type, dir, active);
    rm->ConcurrentStreamStatus(type, dir, active);
}

/*
 * pal_init - Initialize PAL
 *
 * Return 0 on success or error code otherwise
 *
 * Prerequisites
 *    None.
 */
int32_t pal_init(void)
{
    PAL_DBG(LOG_TAG, "Enter.");
    int32_t ret = 0;
    std::shared_ptr<ResourceManager> ri = NULL;
    try {
        ri = ResourceManager::getInstance();
    } catch (const std::exception& e) {
        PAL_ERR(LOG_TAG, "pal init failed: %s", e.what());
        return -EINVAL;
    }
    ret = ri->initSndMonitor();
    if (ret != 0) {
        PAL_ERR(LOG_TAG, "snd monitor init failed");
        return ret;
    }

    ri->init();

    PAL_DBG(LOG_TAG, "Exit. ret : %d ", ret);
    return ret;
}

/*
 * pal_deinit - De-initialize PAL
 *
 * Prerequisites
 *    PAL must be initialized.
 */
void pal_deinit(void)
{
    PAL_INFO(LOG_TAG, "Enter.");
    ResourceManager::deinit();
    PAL_INFO(LOG_TAG, "Exit.");
    return;
}


int32_t pal_stream_open(struct pal_stream_attributes *attributes,
                        uint32_t no_of_devices, struct pal_device *devices,
                        uint32_t no_of_modifiers, struct modifier_kv *modifiers,
                        pal_stream_callback cb, void *cookie,
                        pal_stream_handle_t **stream_handle)
{
    uint64_t *stream = NULL;
    Stream *s = NULL;
    int status;

    if (!attributes) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }

    PAL_INFO(LOG_TAG, "Enter.");

    try {
        s = Stream::create(attributes, devices, no_of_devices, modifiers,
                           no_of_modifiers);
    } catch (const std::exception& e) {
        PAL_ERR(LOG_TAG, "Stream create failed: %s", e.what());
        return -EINVAL;
    }
    if (!s) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "stream creation failed status %d", status);
        return status;
    }
    status = s->open();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_stream_open failed with status %d", status);
        if (s->close() != 0) {
            PAL_ERR(LOG_TAG, "stream closed failed.");
        }
        delete s;
        return status;
    }

    if (cb)
       s->registerCallBack(cb, cookie);
    stream = reinterpret_cast<uint64_t *>(s);
    *stream_handle = stream;
    PAL_INFO(LOG_TAG, "Exit. Value of stream_handle %pK, status %d", stream, status);
    return status;
}

int32_t pal_stream_close(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s = reinterpret_cast<Stream *>(stream_handle);

    status = s->close();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "stream closed failed. status %d", status);
        return status;
    }

    delete s;
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_start(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;
    pal_stream_type_t type;
    pal_stream_direction_t dir;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle %pK", stream_handle);

    s =  reinterpret_cast<Stream *>(stream_handle);

    status = s->start();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "stream start failed. status %d", status);
        return status;
    }

    s->getStreamType(&type);
    s->getStreamDirection(&dir);
    notify_concurrent_stream(type, dir, true);
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_stop(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;
    pal_stream_type_t type;
    pal_stream_direction_t dir;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    s->getStreamType(&type);
    s->getStreamDirection(&dir);

    status = s->stop();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "stream stop failed. status : %d", status);
        notify_concurrent_stream(type, dir, false);
        return status;
    }

    notify_concurrent_stream(type, dir, false);

    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

ssize_t pal_stream_write(pal_stream_handle_t *stream_handle, struct pal_buffer *buf)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle || !buf) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }
    PAL_VERBOSE(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->write(buf);
    if (status < 0) {
        PAL_ERR(LOG_TAG, "stream write failed status %d", status);
        return status;
    }
    PAL_VERBOSE(LOG_TAG, "Exit. status %d", status);
    return status;
}

ssize_t pal_stream_read(pal_stream_handle_t *stream_handle, struct pal_buffer *buf)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle || !buf) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->read(buf);
    if (status < 0) {
        PAL_ERR(LOG_TAG, "stream read failed status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_get_param(pal_stream_handle_t *stream_handle,
                             uint32_t param_id, pal_param_payload **param_payload)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG,  "Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->getParameters(param_id, (void **)param_payload);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "get parameters failed status %d param_id %u", status, param_id);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_set_param(pal_stream_handle_t *stream_handle, uint32_t param_id,
                             pal_param_payload *param_payload)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG,  "Invalid stream handle, status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->setParameters(param_id, (void *)param_payload);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "set parameters failed status %d param_id %u", status, param_id);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_set_volume(pal_stream_handle_t *stream_handle,
                              struct pal_volume_data *volume)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle || !volume) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG,"Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->setVolume(volume);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "setVolume failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_set_mute(pal_stream_handle_t *stream_handle, bool state)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->setMute(state);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "setMute failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_pause(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->setPause();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_stream_pause failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_resume(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_DBG(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    PAL_INFO(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);

    status = s->setResume();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "resume failed with status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_drain(pal_stream_handle_t *stream_handle, pal_drain_type_t type)
{
    Stream *s = NULL;
    int status;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);

    status = s->drain(type);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "drain failed with status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_flush(pal_stream_handle_t *stream_handle)
{
    Stream *s = NULL;
    int status;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);

    status = s->flush();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "flush failed with status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_set_buffer_size (pal_stream_handle_t *stream_handle,
                                    size_t *in_buf_size, const size_t in_buf_count,
                                    size_t *out_buf_size, const size_t out_buf_count)
{
   Stream *s = NULL;
   int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->setBufInfo(in_buf_size,in_buf_count,out_buf_size,out_buf_count);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_stream_set_buffer_size failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_get_timestamp(pal_stream_handle_t *stream_handle,
                          struct pal_session_time *stime)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d\n", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK\n", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->getTimestamp(stime);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_get_timestamp failed with status %d\n", status);
        return status;
    }
    PAL_VERBOSE(LOG_TAG, "stime->session_time.value_lsw = %u, stime->session_time.value_msw = %u \n", stime->session_time.value_lsw, stime->session_time.value_msw);
    PAL_VERBOSE(LOG_TAG, "stime->absolute_time.value_lsw = %u, stime->absolute_time.value_msw = %u \n", stime->absolute_time.value_lsw, stime->absolute_time.value_msw);
    PAL_VERBOSE(LOG_TAG, "stime->timestamp.value_lsw = %u, stime->timestamp.value_msw = %u \n", stime->timestamp.value_lsw, stime->timestamp.value_msw);
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_add_remove_effect(pal_stream_handle_t *stream_handle,
                       pal_audio_effect_t effect, bool enable)
{
    Stream *s = NULL;
    int status = EINVAL;
    pal_stream_type_t type;

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);

    status = s->getStreamType(&type);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamType failed with status = %d", status);
        return status;
    }
    if (PAL_STREAM_VOIP_TX == type) {
        s =  reinterpret_cast<Stream *>(stream_handle);
        status = s->addRemoveEffect(effect, enable);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "pal_add_effect failed with status %d", status);
            return status;
        }
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;

}
int32_t pal_stream_set_device(pal_stream_handle_t *stream_handle,
                           uint32_t no_of_devices, struct pal_device *devices)
{
    int status = -EINVAL;
    Stream *s = NULL;
    std::shared_ptr<ResourceManager> rm = NULL;
    struct pal_stream_attributes sattr;
    struct pal_device_info devinfo = {};

    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream handle status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);

    if (!devices) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid device status %d", status);
        return status;
    }

    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Invalid resource manager");
        return status;
    }

    /* Choose best device config for this stream */
    /* TODO: Decide whether to update device config or not based on flag */
    s =  reinterpret_cast<Stream *>(stream_handle);
    s->getStreamAttributes(&sattr);

    // device switch will be handled in global param setting for SVA
    if (sattr.type == PAL_STREAM_VOICE_UI) {
        PAL_DBG(LOG_TAG,
                "Device switch handles in global param set, skip here");
        return status;
    }

    for (int i = 0; i < no_of_devices; i++) {
        rm->getDeviceInfo(devices[i].id, sattr.type, &devinfo);
        if (devinfo.channels == 0 || devinfo.channels > devinfo.max_channels) {
            PAL_ERR(LOG_TAG, "Num channels[%d] is invalid", devinfo.channels);
            return -EINVAL;
        }
        status = rm->getDeviceConfig((struct pal_device *)&devices[i], &sattr, devinfo.channels);
        if (status) {
           PAL_ERR(LOG_TAG, "Failed to get Device config, err: %d", status);
           return status;
        }
    }
    // TODO: Check with RM if the same device is being used by other stream with different
    // configuration then update corresponding stream device configuration also based on priority.
    PAL_DBG(LOG_TAG, "Stream handle :%pK no_of_devices %d first_device id %d",
            stream_handle, no_of_devices, devices[0].id);

    status = s->switchDevice(s, no_of_devices, devices);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "failed with status %d", status);
        return status;
    }

    PAL_DBG(LOG_TAG, "Exit. status %d", status);

    return status;
}

int32_t pal_set_param(uint32_t param_id, void *param_payload,
                      size_t payload_size)
{
    PAL_DBG(LOG_TAG, "Enter:");
    int status = 0;
    std::shared_ptr<ResourceManager> rm = NULL;

    rm = ResourceManager::getInstance();

    if (rm) {
        status = rm->setParameter(param_id, param_payload, payload_size);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to set global parameter %u, status %d",
                    param_id, status);
        }
    } else {
        PAL_ERR(LOG_TAG, "Pal has not been initialized yet");
        status = -EINVAL;
    }
    PAL_DBG(LOG_TAG, "Exit:");
    return status;
}

int32_t pal_get_param(uint32_t param_id, void **param_payload,
                      size_t *payload_size, void *query)
{
    int status = 0;
    std::shared_ptr<ResourceManager> rm = NULL;

    rm = ResourceManager::getInstance();

    PAL_DBG(LOG_TAG, "Enter:");

    if (rm) {
        status = rm->getParameter(param_id, param_payload, payload_size, query);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to get global parameter %u, status %d",
                    param_id, status);
        }
    } else {
        PAL_ERR(LOG_TAG, "Pal has not been initialized yet");
        status = -EINVAL;
    }
    PAL_DBG(LOG_TAG, "Exit: %d", status);
    return status;
}

int32_t pal_stream_get_mmap_position(pal_stream_handle_t *stream_handle,
                              struct pal_mmap_position *position)
{
   Stream *s = NULL;
   int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->GetMmapPosition(position);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_stream_set_buffer_size failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_stream_create_mmap_buffer(pal_stream_handle_t *stream_handle,
                              int32_t min_size_frames,
                              struct pal_mmap_buffer *info)
{
    Stream *s = NULL;
    int status;
    if (!stream_handle) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid input parameters status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Enter. Stream handle :%pK", stream_handle);
    s =  reinterpret_cast<Stream *>(stream_handle);
    status = s->createMmapBuffer(min_size_frames, info);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "pal_stream_set_buffer_size failed with status %d", status);
        return status;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int32_t pal_register_global_callback(pal_global_callback cb, void *cookie)
{
    std::shared_ptr<ResourceManager> rm = NULL;

    PAL_DBG(LOG_TAG, "Enter. global callback %pK, cookie %pK", cb, cookie);
    rm = ResourceManager::getInstance();

    if (cb != NULL) {
        rm->globalCb = cb;
        rm->cookie = cookie;
    }
    return 0;
}

int32_t pal_gef_rw_param(uint32_t param_id, void *param_payload,
                      size_t payload_size, pal_device_id_t pal_device_id,
                      pal_stream_type_t pal_stream_type, unsigned int dir)
{
    int status = 0;
    std::shared_ptr<ResourceManager> rm = NULL;

    rm = ResourceManager::getInstance();

    PAL_DBG(LOG_TAG, "Enter.");

    if (rm) {
        if (GEF_PARAM_WRITE == dir) {
            status = rm->setParameter(param_id, param_payload, payload_size,
                                        pal_device_id, pal_stream_type);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set global parameter %u, status %d",
                        param_id, status);
            }
        } else {
            status = rm->getParameter(param_id, param_payload, payload_size,
                                        pal_device_id, pal_stream_type);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set global parameter %u, status %d",
                        param_id, status);
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Pal has not been initialized yet");
        status = -EINVAL;
    }
    PAL_DBG(LOG_TAG, "Exit:");

    return status;
}