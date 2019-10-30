/*------------------------------------------------------------------------------------------------*/
/* Customers INIC automated reprogramming logic                                                   */
/* Copyright 2017, Microchip Technology Inc. and its subsidiaries.                                */
/*                                                                                                */
/* Redistribution and use in source and binary forms, with or without                             */
/* modification, are permitted provided that the following conditions are met:                    */
/*                                                                                                */
/* 1. Redistributions of source code must retain the above copyright notice, this                 */
/*    list of conditions and the following disclaimer.                                            */
/*                                                                                                */
/* 2. Redistributions in binary form must reproduce the above copyright notice,                   */
/*    this list of conditions and the following disclaimer in the documentation                   */
/*    and/or other materials provided with the distribution.                                      */
/*                                                                                                */
/* 3. Neither the name of the copyright holder nor the names of its                               */
/*    contributors may be used to endorse or promote products derived from                        */
/*    this software without specific prior written permission.                                    */
/*                                                                                                */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"                    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                      */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                 */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE                   */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                     */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR                     */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER                     */
/* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,                  */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE                  */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/*------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "ucsi_api.h"

/************************************************************************/
/* Private Function Prototypes                                          */
/************************************************************************/

static bool HasNodeAddressConflict(uint16_t checkNodeAddress, const Ucs_Signature_t *pCheckNode, const Ucs_Signature_t *pAllNodes, uint32_t nodeArrayLen);
static bool HasMacAddressConflict(uint64_t checkMacAddress, const Ucs_Signature_t *pCheckNode, const Ucs_Signature_t *pAllNodes, uint32_t nodeArrayLen);
static uint64_t TranslateMacAddressToInt(const Ucs_Signature_t *signature);

/************************************************************************/
/* Public Function Implementations                                      */
/************************************************************************/

/**
 * \brief Callback when programming mode is active and the needed amount of devices where discovered in the network.
 * \note This function must be implemented by the integrator
 * \param pTag - Pointer given by the integrator by UCSI_Init
 * \param pNodes - Array of signature pointers of all the discovered devices in the network.
 * \param nodeArrayLen - The length of the array
 * \param pNewIdentString - Valid pointer, user can specify the parameters for one node. After programming of that node, this function will be called again. Until the integrator returns false.
 * \return true, the user filled pNewIdentString structure, the given node shall be reprogrammed. false, no change, leave programming mode and go into normal mode.
 */
const Ucs_Signature_t *UCSI_CB_OnProgrammingModeDeviceDiscovery(void *pTag, const Ucs_Signature_t *pNodes, uint32_t nodeArrayLen, Ucs_IdentString_t *pNewIdentString)
{
    int32_t i;
    const Ucs_Signature_t *nodeToBeFlashed = NULL;
    bool needToProgram = false;
    uint16_t node = 0xCCCC;
    uint16_t group = 0xCCCC;
    uint64_t mac  = 0xCCCCCCCCCCCCCCCCull;
    if (!pNodes || nodeArrayLen < 2 || !pNewIdentString) {
        assert(false);
        return NULL;
    }
    for (i = 0; i < nodeArrayLen && !needToProgram; i++) {
        nodeToBeFlashed = &pNodes[i];
        group = pNodes[i].group_address;
        /* NODE ADDRESS CHECK */
        if (HasNodeAddressConflict(pNodes[i].node_address, &pNodes[i], pNodes, nodeArrayLen)) {
            node = pNodes[i].diagnosis_id;
            if (!node)
                node = pNodes[i].node_address;
            while(HasNodeAddressConflict(node, &pNodes[i], pNodes, nodeArrayLen)) {
                node += 1;
            }
            needToProgram = true;
        }
        /* MAC ADDRESS CHECK */
        mac = TranslateMacAddressToInt(&pNodes[i]);
        if (HasMacAddressConflict(mac, &pNodes[i], pNodes, nodeArrayLen)) {
            do {
                mac += 1;
            } while(HasMacAddressConflict(mac, &pNodes[i], pNodes, nodeArrayLen));
            needToProgram = true;
        }
    }
    if (!needToProgram)
        return NULL;
    pNewIdentString->node_address = node;
    pNewIdentString->group_address = group;
    pNewIdentString->mac_47_32 = (mac >> 32);
    pNewIdentString->mac_31_16 = (mac >> 16);
    pNewIdentString->mac_15_0 = mac;
    return nodeToBeFlashed;
}

/************************************************************************/
/* Private Functions                                                    */
/************************************************************************/

static bool HasNodeAddressConflict(uint16_t checkNodeAddress, const Ucs_Signature_t *pCheckNode, const Ucs_Signature_t *pAllNodes, uint32_t nodeArrayLen)
{
    bool hasConflict = false;
    uint32_t i;
    assert(checkNodeAddress);
    assert(pCheckNode);
    assert(pAllNodes);
    assert(nodeArrayLen);
    for (i = 0; i < nodeArrayLen; i++)
    {
        const Ucs_Signature_t *pCurNode = &pAllNodes[i];
        if (pCheckNode == pCurNode)
            continue;
        if (checkNodeAddress == pCurNode->node_address) {
            hasConflict = true;
            break;
        }
    }
    return hasConflict;
}

static bool HasMacAddressConflict(uint64_t checkMacAddress, const Ucs_Signature_t *pCheckNode, const Ucs_Signature_t *pAllNodes, uint32_t nodeArrayLen)
{
    bool hasConflict = false;
    uint32_t i;
    assert(pCheckNode);
    assert(pAllNodes);
    assert(nodeArrayLen);
    if (0 == checkMacAddress)
        return false;
    for (i = 0; i < nodeArrayLen; i++)
    {
        const Ucs_Signature_t *pCurNode = &pAllNodes[i];
        if (pCheckNode == pCurNode)
            continue;
        if (checkMacAddress == TranslateMacAddressToInt(pCurNode)) {
            hasConflict = true;
            break;
        }
    }
    return hasConflict;
}

static uint64_t TranslateMacAddressToInt(const Ucs_Signature_t *signature)
{
    uint64_t val;
    if (NULL == signature)
        return 0;
    val = (uint64_t)signature->mac_47_32 << 32;
    val |= (uint64_t)signature->mac_31_16 << 16;
    val |= (uint64_t)signature->mac_15_0;
    return val;
}