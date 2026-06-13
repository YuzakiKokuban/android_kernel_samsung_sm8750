/*
 * TISpeakerProtDefs.h
 *
 *  Created on: Oct 10, 2023
 *      Author: MASHKURAHMB
 */

#ifndef PAL_INC_TI_SPEAKERPROT_DEFS_H_
#define PAL_INC_TI_SPEAKERPROT_DEFS_H_

#ifndef SPEAKER_PROT
struct agmMetaData {
    uint8_t *buf;
    uint32_t size;
    agmMetaData(uint8_t *b, uint32_t s)
        :buf(b),size(s) {}
};
#endif

#define PAL_TISA_SET_PARAM   0
#define PAL_TISA_GET_PARAM   1

#define PAL_TISA_PARAM_GEN_SETPARAM  0x71545 /*TISAS*/
#define PAL_TISA_PARAM_GEN_GETPARAM  0x71546 /*TISAG*/

#define AUDIO_PARAM_MAX_LEN 11
#define AUDIO_PARAM_HDR_LEN 4

typedef struct pal_tispk_prot_param_t {
    int hdr[AUDIO_PARAM_HDR_LEN]; /*ch, index, len, isget */
    int data[AUDIO_PARAM_MAX_LEN];
} pal_tispk_prot_param_t;

#define AUDIO_PARAM_TI_SMARTPA      "ti_spkprot"
#define AUDIO_PARAM_TI_SMARTPA_CH   "ti_channel"
#define AUDIO_PARAM_TI_SMARTPA_IDX  "ti_idx"
#define AUDIO_PARAM_TI_SMARTPA_LEN  "ti_len"
#define AUDIO_PARAM_TI_SMARTPA_GET  "ti_get"

enum {
    AUDIO_PARAM_TI_SMARTPA_CH_IDX = 0,
    AUDIO_PARAM_TI_SMARTPA_IDX_IDX,
    AUDIO_PARAM_TI_SMARTPA_LEN_IDX,
    AUDIO_PARAM_TI_SMARTPA_GET_IDX
};
#endif /* PAL_INC_TI_SPEAKERPROT_DEFS_H_ */
