/**
 ****************************************************************************************
 *
 * @file aes.c
 *
 * @brief AES implementation.
 *
 * Copyright (C) 2017-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
 * MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES. USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN
 * AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE TERMS
 * OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT
 * SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE TERMS OF
 * THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT PERMITTED TO USE THIS
 * SOFTWARE.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup aes
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <string.h>
#include <stdint.h>
#include "aes.h"
#include "ke_task.h"
#include "aes_task.h"
#include "llm_task.h"
#include "ke_mem.h"
#include "sw_aes.h"
#include "aes_api.h"
#include "co_bt.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct aes_env_tag aes_env      __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
void (*aes_cb)(uint8_t status)  __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY

uint8_t aes_out[ENC_DATA_LEN];

/* using a dummy IV vector filled with zeroes for decryption since the chip does not use IV at encryption */
uint8_t IV[ENC_DATA_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/// TASK_AES descriptor
static const struct ke_task_desc TASK_DESC_AES = {aes_state_handler,
                                                  &aes_default_handler,
                                                  aes_state,
                                                  AES_STATE_MAX,
                                                  AES_IDX_MAX};

/*
 * PUBLIC FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void aes_init(bool reset, void (*aes_done_cb)(uint8_t status))
{
    // Check that numbers are in order. If not check enum gapm_operation.
    ASSERT_WARNING (AES_OP_GEN_RAND_NB == AES_OP_USE_ENC_BLOCK + 1);

    aes_cb = aes_done_cb;

    if(!reset)
    {
        // Reset the aes environment
        memset(&aes_env, 0, sizeof(aes_env));

        // Create aes task
        ke_task_create(TASK_AES, &TASK_DESC_AES);
    }
    else
    {
        if(aes_env.operation != NULL)
        {
            KE_MSG_FREE(ke_param2msg(aes_env.operation));
        }

        // Reset the aes environment
        memset(&aes_env, 0, sizeof(aes_env));
    }

    //default initial state is IDLE
    ke_state_set(TASK_AES, AES_IDLE);
}

/**
 ****************************************************************************************
 * @brief AES partial operation. Encrypt/decrypt a 16 byte block. Uses the AES environment
 *        for different block parameters.
 * @param[in] enc_dec       AES_ENCRYPT for encryption, AES_DECRYPT for decryption.
 * @return                   0 if successfull,
 *                          -1 if userKey or key are NULL,
 *                          -2 if bits is not 128,
 *                          -3 if enc_dec not 0/1.
 ****************************************************************************************
 */
static int aes_part(uint8_t enc_dec)
{
    int status = 0;
    struct aes_cmp_evt *evt;

    if ((enc_dec != AES_ENCRYPT) && (enc_dec != AES_DECRYPT))
    {
        return -3;
    }

sync_loop:

    status = aes_enc_dec(&aes_env.in[aes_env.in_cur], &aes_env.out[aes_env.out_cur],
                         &aes_env.aes_key, enc_dec, aes_env.ble_flags);
    if(status == 0)
        aes_env.in_cur += ENC_DATA_LEN;

    if((aes_env.ble_flags & BLE_SYNC_MASK))     //use synchronous mode
    {
        if(status == 0 && aes_env.in_cur < aes_env.in_len)
        {
            aes_env.out_cur += ENC_DATA_LEN;
            goto sync_loop;
        }
    }
    else   //use asynchronous, message based mode of operation
    {
        // Allocate the message for the response
        evt = KE_MSG_ALLOC(AES_CMP_EVT, TASK_AES, TASK_LLM, aes_cmp_evt);
        // Set the status
        evt->status = status;

        // Send the event
        KE_MSG_SEND(evt);
    }

    return status;
}

int aes_operation(uint8_t *key, uint8_t key_len, uint8_t *in, uint32_t in_len, uint8_t *out, uint32_t out_len, uint8_t enc_dec, void (*aes_done_cb)(uint8_t status), uint8_t ble_flags)
{
    if(aes_env.operation != 0)  //busy
        return -2;

    if(key_len != ENC_DATA_LEN)
        return -4;

    aes_env.key = key;
    aes_env.key_len = key_len;
    aes_env.in = in;
    aes_env.in_len = in_len;
    aes_env.out = out;
    aes_env.out_len = out_len;

    aes_env.in_cur = 0;
    aes_env.out_cur = 0;
    aes_env.enc_dec = enc_dec;
    aes_env.aes_done_cb = aes_done_cb;
    aes_env.ble_flags = ble_flags;

    if((ble_flags & BLE_SYNC_MASK) == 0)   //use asynchronous, message based mode of operation
    {
        struct gapm_use_enc_block_cmd *cmd = KE_MSG_ALLOC(AES_USE_ENC_BLOCK_CMD,
                                                          TASK_AES,
                                                          TASK_AES,
                                                          gapm_use_enc_block_cmd);

        cmd->operation = enc_dec ? AES_OP_USE_ENC_BLOCK : AES_OP_GEN_RAND_NB;

        // Go in a busy state
        ke_state_set(TASK_AES, AES_BUSY);

        // Keep the command message until the end of the request
        aes_env.operation = (void *)cmd;
        aes_env.requester = TASK_AES;
    }

    aes_set_key(aes_env.key, 128, &aes_env.aes_key, aes_env.enc_dec);

    return aes_part(enc_dec);
}

void aes_send_cmp_evt(ke_task_id_t cmd_src_id, uint8_t operation, uint8_t status)
{
    //status is OK so continue
    if(status == SMP_ERROR_NO_ERROR && aes_env.in_cur < aes_env.in_len) //more bytes
    {
        aes_env.out_cur += ENC_DATA_LEN;

        if(aes_part(aes_env.enc_dec) == 0)
            return;
        else
            status = -status;
    }

    // Come back to IDLE state
    ke_state_set(TASK_AES, AES_IDLE);

    // Release the command message
    if (aes_env.operation != NULL)
    {
        KE_MSG_FREE(ke_param2msg(aes_env.operation));
        aes_env.operation = NULL;
    }

    if(aes_env.aes_done_cb == NULL) //no callback so do nothing
        return;

    aes_env.aes_done_cb(status);
}

void aes_send_encrypt_req(uint8_t *operand_1, uint8_t *operand_2)
{
    uint8_t enc_dec = ((struct gapm_use_enc_block_cmd *)(aes_env.operation))->operation == AES_OP_USE_ENC_BLOCK ? 1 : 0;

    aes_operation(operand_1, ENC_DATA_LEN, operand_2, ENC_DATA_LEN, aes_out, ENC_DATA_LEN, enc_dec, aes_cb, 0);
}
/// @} aes
