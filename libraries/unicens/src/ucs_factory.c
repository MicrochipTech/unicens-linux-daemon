/*------------------------------------------------------------------------------------------------*/
/* UNICENS V2.1.0-3564                                                                            */
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

/*!
 * \file
 * \brief Implementation of the UCS Factory.
 *
 * \cond UCS_INTERNAL_DOC
 * \addtogroup G_FAC
 * @{
 */

/*------------------------------------------------------------------------------------------------*/
/* Includes                                                                                       */
/*------------------------------------------------------------------------------------------------*/
#include "ucs_factory.h"
#include "ucs_xrm_pv.h"

/*------------------------------------------------------------------------------------------------*/
/* Internal macros                                                                                */
/*------------------------------------------------------------------------------------------------*/
#define IS_VALID_ADDR(addr) ((UCS_ADDR_LOCAL_DEV == (addr)) || ((0x0FU < (addr)) && (0x300U > (addr))) || ((0x04FFU < (addr)) && (0x0FF0U > (addr)))) /* parasoft-suppress  MISRA2004-19_7 "common definition of type cast improves code" */

/*------------------------------------------------------------------------------------------------*/
/* Internal prototypes                                                                            */
/*------------------------------------------------------------------------------------------------*/
static void Fac_ConstructFbi (CFactory * self, CInic * fbi, uint16_t address);
static CInic * Fac_SearchFbi(CFactory * self, uint16_t address);
static void Fac_ConstructNsm (CFactory * self, CNodeScriptManagement * nsm_ptr, CRemoteSyncManagement * rsm_ptr);
static CRemoteSyncManagement * Fac_SearchRsm(CFactory * self, uint16_t address);
static CExtendedResourceManager * Fac_SearchXrm(CFactory * self, uint16_t address);
static CGpio * Fac_SearchGpio(CFactory * self, uint16_t address);
static CI2c* Fac_SearchI2c(CFactory * self, uint16_t address);
static CNodeScriptManagement * Fac_SearchNsm(CFactory * self, uint16_t address);
static CInic * Fac_GetUninitializedFbi (CFactory * self);
static CNodeScriptManagement * Fac_GetUninitializedNsm (CFactory * self);
static CRemoteSyncManagement * Fac_GetUninitializedRsm (CFactory * self);
static CExtendedResourceManager * Fac_GetUninitializedXrm (CFactory * self);
static CGpio * Fac_GetUninitializedGpio (CFactory * self);
static CI2c * Fac_GetUninitializedI2c (CFactory * self);
static bool Fac_IsFbiUninitialized(CInic * fbi);
static bool Fac_IsRsmUninitialized(CRemoteSyncManagement * rsm);
static bool Fac_IsXrmUninitialized(CExtendedResourceManager * xrm);
static bool Fac_IsGpioUninitialized(CGpio * gpio);
static bool Fac_IsI2cUninitialized(CI2c * i2c);
static bool Fac_IsNsmUninitialized(CNodeScriptManagement * nsm);

/*------------------------------------------------------------------------------------------------*/
/* Implementation of class CFactory                                                               */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Constructor of the UCS Factory class.
 *  \param self        Instance pointer
 *  \param init_ptr    init data_ptr
 */
void Fac_Ctor(CFactory * self, Fac_InitData_t * init_ptr)
{
    uint8_t i;
    Rsm_InitData_t rsm_init_data;

    MISC_MEM_SET(self, 0, sizeof(CFactory));

    /* set base and net instances */
    self->base_ptr = init_ptr->base_ptr;
    self->net_ptr  = init_ptr->net_ptr;
    self->xrmp_ptr = init_ptr->xrmp_ptr;
    self->icm_transceiver = init_ptr->icm_transceiver;
    self->rcm_transceiver = init_ptr->rcm_transceiver;

    rsm_init_data.base_ptr = self->base_ptr;
    rsm_init_data.net_ptr  = self->net_ptr;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        rsm_init_data.inic_ptr = &self->fbi_list[i];
        Rsm_Ctor(&self->rsm_list[i], &rsm_init_data);
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Service                                                                                        */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Returns the XRM instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \param res_debugging_fptr        The resources debugging callback function
 *  \param check_unmute_fptr         The check unmute callback function
 *  \return a reference to a XRM instance or \c NULL if no appropriate instance has been found.
 */
CExtendedResourceManager * Fac_GetXrm(CFactory * self, uint16_t address, Ucs_Xrm_ResourceDebugCb_t res_debugging_fptr, Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr)
{
    CRemoteSyncManagement * rsm_inst = NULL;
    CExtendedResourceManager * xrm_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        xrm_inst = Fac_SearchXrm(self, address);
        if (xrm_inst == NULL)
        {
            rsm_inst = Fac_GetRsm(self, address);
            if (rsm_inst != NULL)
            {
                Xrm_InitData_t xrm_init_data;
                xrm_inst = Fac_GetUninitializedXrm(self);
                if (xrm_inst != NULL)
                {
                    xrm_init_data.base_ptr = self->base_ptr;
                    xrm_init_data.net_ptr  = self->net_ptr;
                    xrm_init_data.rsm_ptr  = rsm_inst;
                    xrm_init_data.inic_ptr = rsm_inst->inic_ptr;
                    xrm_init_data.xrmp_ptr = self->xrmp_ptr;
                    xrm_init_data.check_unmute_fptr  = check_unmute_fptr;
                    xrm_init_data.res_debugging_fptr = res_debugging_fptr;
                    Xrm_Ctor(xrm_inst, &xrm_init_data);
                }
            }
        }
        Xrm_SetResourceDebugCbFn(xrm_inst, res_debugging_fptr);
    }

    return xrm_inst;
}

/*! \brief Returns the XRM instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \param check_unmute_fptr         The check unmute callback function
 *  \return a reference to a XRM instance or \c NULL if no appropriate instance has been found.
 */
CExtendedResourceManager * Fac_GetXrmLegacy(CFactory * self, uint16_t address, Ucs_Xrm_CheckUnmuteCb_t check_unmute_fptr)
{
    return Fac_GetXrm(self, address, NULL, check_unmute_fptr);
}


/*! \brief Returns the XRM instance associated with the resource list.
 *  \note    <b>This function should only be used in case of Ucs_Xrm_Destroy() since it's certain in that case that the XRM instance for the given job list already exists!</b>
 *  \param self                      Instance pointer
 *  \param resource_object_list      Reference to the job list
 *  \return a reference to a XRM instance or \c NULL if no appropriate instance has been found.
 */
CExtendedResourceManager * Fac_GetXrmByJobList(CFactory * self, UCS_XRM_CONST Ucs_Xrm_ResObject_t *resource_object_list[])
{
    uint8_t i;
    CExtendedResourceManager * ret_xrm = NULL;

    for(i=0U; i<FAC_NUM_DEVICES; i++)
    {
        if (Xrm_IsInMyJobList(&self->xrm_list[i], resource_object_list))
        {
            ret_xrm = &self->xrm_list[i];
            break;
        }
    }

    return ret_xrm;
}

/*! \brief Returns the FBlock INIC instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \return a reference to a FBI instance or \c NULL if no suitable instance has been found.
 */
CInic * Fac_GetInic(CFactory * self, uint16_t address)
{
    CInic * fbi_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        fbi_inst = Fac_SearchFbi(self, address);
        if (fbi_inst == NULL)
        {
            fbi_inst = Fac_GetUninitializedFbi(self);
            if (fbi_inst != NULL)
            {
                Fac_ConstructFbi(self, fbi_inst, address);
            }
        }
    }

    return fbi_inst;
}

/*! \brief Returns the CNodeScriptManagement instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \return a reference to a FBI instance or \c NULL if no suitable instance has been found.
 */
CNodeScriptManagement * Fac_GetNsm(CFactory * self, uint16_t address)
{
    CNodeScriptManagement * nsm_inst = NULL;
    CRemoteSyncManagement * rsm_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        nsm_inst = Fac_SearchNsm(self, address);
        if (nsm_inst == NULL)
        {
            rsm_inst = Fac_GetRsm(self, address);
            if (rsm_inst != NULL)
            {
                nsm_inst = Fac_GetUninitializedNsm(self);
                if (nsm_inst != NULL)
                {
                    Fac_ConstructNsm(self, nsm_inst, rsm_inst);
                }
            }
        }
    }

    return nsm_inst;
}

/*! \brief Returns the RSM instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \return a reference to a RSM instance or \c NULL if no suitable instance has been found.
 */
CRemoteSyncManagement * Fac_GetRsm(CFactory * self, uint16_t address)
{
    CRemoteSyncManagement * rsm_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        rsm_inst = Fac_SearchRsm(self, address);
        if (rsm_inst == NULL)
        {
            rsm_inst = Fac_GetUninitializedRsm(self);
            if (rsm_inst != NULL)
            {
                Fac_ConstructFbi(self, rsm_inst->inic_ptr, address);
            }
        }
    }

    return rsm_inst;
}

/*! \brief Returns the GPIO instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \param trigger_event_status_fptr User GPIO trigger event status callback function pointer.
 *  \return a reference to a GPIO instance or \c NULL if no suitable instance has been found.
 */
CGpio * Fac_GetGpio(CFactory * self, uint16_t address, Ucs_Gpio_TriggerEventResultCb_t trigger_event_status_fptr)
{
    CGpio * gpio_inst = NULL;
    CNodeScriptManagement * nsm_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        gpio_inst = Fac_SearchGpio(self, address);
        if (NULL == gpio_inst)
        {
            nsm_inst = Fac_GetNsm(self, address);
            if (NULL != nsm_inst)
            {
                Gpio_InitData_t gpio_init_data;
                gpio_inst = Fac_GetUninitializedGpio(self);
                if (NULL != gpio_inst)
                {
                    gpio_init_data.nsm_ptr  = nsm_inst;
                    gpio_init_data.inic_ptr = nsm_inst->rsm_ptr->inic_ptr;
                    gpio_init_data.trigger_event_status_fptr = trigger_event_status_fptr;
                    Gpio_Ctor(gpio_inst, &gpio_init_data);
                }
            }
        }
    }

    return gpio_inst;
}

/*! \brief Returns the I2C instance associated with the given address.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with the instance
 *  \param i2c_interrupt_report_fptr User GPIO trigger event status callback function pointer.
 *  \return a reference to an I2C instance or \c NULL if no suitable instance has been found.
 */
CI2c * Fac_GetI2c(CFactory * self, uint16_t address, Ucs_I2c_IntEventReportCb_t i2c_interrupt_report_fptr)
{
    CI2c * i2c_inst = NULL;
    CNodeScriptManagement * nsm_inst = NULL;

    if (IS_VALID_ADDR(address))
    {
        i2c_inst = Fac_SearchI2c (self, address);
        if (NULL == i2c_inst)
        {
            nsm_inst = Fac_GetNsm(self, address); 
            if (nsm_inst != NULL)
            {
                I2c_InitData_t i2c_init_data;
                i2c_inst = Fac_GetUninitializedI2c(self);
                if (NULL != i2c_inst)
                {
                    i2c_init_data.nsm_ptr  = nsm_inst;
                    i2c_init_data.inic_ptr = nsm_inst->rsm_ptr->inic_ptr;
                    i2c_init_data.i2c_interrupt_report_fptr = i2c_interrupt_report_fptr;
                    I2c_Ctor(i2c_inst, &i2c_init_data);
                }
            }
        }
    }

    return i2c_inst;
}

/*! \brief Searches for the INIC instance associated with the given address and returns It if found.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with this instance
 *  \return a reference to the found instance otherwise \c NULL.
 */
CInic * Fac_FindInic(CFactory * self, uint16_t address)
{
    return Fac_SearchFbi (self, address);
}

/*! \brief Searches for the NSM instance associated with the given address and returns It if found.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with this instance
 *  \return a reference to the found instance otherwise \c NULL.
 */
CNodeScriptManagement * Fac_FindNsm(CFactory * self, uint16_t address)
{
    return Fac_SearchNsm (self, address);
}

/*! \brief Searches for the RSM instance associated with the given address and returns It if found.
 *  \param self                      Instance pointer
 *  \param address                   Address of the device associated with this instance
 *  \return a reference to the found instance otherwise \c NULL.
 */
CRemoteSyncManagement * Fac_FindRsm(CFactory * self, uint16_t address)
{
    return Fac_SearchRsm (self, address);
}

/*! \brief  Calls the given function for each instance of inst_type type. If the func_ptr 
 *          returns true the loop is stopped.
 *  \param  self            Reference to a Factory Instance
 *  \param  inst_type       The instance type to be looked for
 *  \param  func_ptr        Reference of the callback function which is called for each node
 *  \param  user_data_ptr   Reference of optional user data pass to the func_ptr
 */
void Fac_Foreach(CFactory * self, Fac_Inst_t inst_type, Fac_ForeachFunc_t func_ptr, void *user_data_ptr)
{
    uint8_t j;
    void * curr_inst = NULL;
    bool exit_loop = false;

    for(j=0U; j<FAC_NUM_DEVICES; j++)
    {
        switch(inst_type)
        {
            case FAC_INST_INIC:
                curr_inst = &self->fbi_list[j];
                if (Fac_IsFbiUninitialized((CInic *)curr_inst))
                {
                    curr_inst = NULL;
                }
                break;

            case FAC_INST_RSM:
                curr_inst = &self->rsm_list[j];
                if (Fac_IsRsmUninitialized((CRemoteSyncManagement *)curr_inst))
                {
                    curr_inst = NULL;
                }
                break;

            case FAC_INST_XRM:
                curr_inst = &self->xrm_list[j];
                if (Fac_IsXrmUninitialized((CExtendedResourceManager *)curr_inst))
                {
                    curr_inst = NULL;
                }
                break;

            case FAC_INST_GPIO:
                curr_inst = &self->gpio_list[j];
                if (Fac_IsGpioUninitialized((CGpio *)curr_inst))
                {
                    curr_inst = NULL;
                }
                break;

            case FAC_INST_I2C:
                curr_inst = &self->i2c_list[j];
                if (Fac_IsI2cUninitialized((CI2c *)curr_inst))
                {
                    curr_inst = NULL;
                }
                break;

            default:
                break;
        }

        if (curr_inst != NULL)
        {
            if (func_ptr(inst_type, curr_inst, user_data_ptr) != false) 
            {
                exit_loop = true;
            }
        }
        else
        {
            exit_loop = true;
        }

        if (exit_loop)
        {
            break;
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Private Methods                                                                                */
/*------------------------------------------------------------------------------------------------*/
/*! \brief Search for the FBI instance associated with the given address and return It.
 *  \param self       Instance pointer
 *  \param address    Address to be looked for
 *  \return a reference to the found FBI or \c NULL if no suitable instance has been found.
 */
static CInic * Fac_SearchFbi(CFactory * self, uint16_t address)
{
    CInic * found_fbi = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsFbiUninitialized(&self->fbi_list[i])); i++)
    {
        if (tmp_addr == Inic_GetTargetAddress(&self->fbi_list[i]))
        {
            found_fbi = &self->fbi_list[i];
            break;
        }
    }

    return found_fbi;
}

/*! \brief Search for the NSM instance associated with the given address and return It.
 *  \param self       Instance pointer
 *  \param address    Address to be looked for
 *  \return a reference to the found NSM or \c NULL if no suitable instance has been found.
 */
static CNodeScriptManagement * Fac_SearchNsm(CFactory * self, uint16_t address)
{
    CNodeScriptManagement * found_nsm = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsNsmUninitialized(&self->nsm_list[i])); i++)
    {
        if (tmp_addr == self->nsm_list[i].target_address)
        {
            found_nsm = &self->nsm_list[i];
            break;
        }
    }

    return found_nsm;
}

/*! \brief Search for the RSM instance associated with the given address.
 *  \param self    Instance pointer
 *  \param address Address to be looked for
 *  \return a reference to the found RSM or \c NULL if no suitable instance has been found.
 */
static CRemoteSyncManagement * Fac_SearchRsm(CFactory * self, uint16_t address)
{
    CRemoteSyncManagement * found_rsm = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsFbiUninitialized(self->rsm_list[i].inic_ptr)); i++)
    {
        if (tmp_addr == Inic_GetTargetAddress(self->rsm_list[i].inic_ptr))
        {
            found_rsm = &self->rsm_list[i];
            break;
        }
    }

    return found_rsm;
}

/*! \brief Search for the XRM instance associated with the given address.
 *  \param self    Instance pointer
 *  \param address    Address to be looked for
 *  \return a reference to the found XRM or \c NULL if no suitable instance has been found.
 */
static CExtendedResourceManager * Fac_SearchXrm(CFactory * self, uint16_t address)
{
    CExtendedResourceManager * found_xrm = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsXrmUninitialized(&self->xrm_list[i])); i++)
    {
        if (tmp_addr == Inic_GetTargetAddress(self->xrm_list[i].rsm_ptr->inic_ptr))
        {
            found_xrm = &self->xrm_list[i];
            break;
        }
    }

    return found_xrm;
}

/*! \brief Search for the Gpio instance associated with the given address.
 *  \param self    Instance pointer
 *  \param address    Address to be looked for
 *  \return a reference to the found GPIO or \c NULL if no suitable instance has been found.
 */
static CGpio * Fac_SearchGpio(CFactory * self, uint16_t address)
{
    CGpio * found_gpio = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsGpioUninitialized(&self->gpio_list[i])); i++)
    {
        if (tmp_addr == Inic_GetTargetAddress(self->gpio_list[i].nsm_ptr->rsm_ptr->inic_ptr))
        {
            found_gpio = &self->gpio_list[i];
            break;
        }
    }

    return found_gpio;
}

/*! \brief Search for the I2c instance associated with the given address.
 *  \param self       Instance pointer
 *  \param address    Address to be looked for
 *  \return a reference to the found GPIO or \c NULL if no suitable instance has been found.
 */
static CI2c * Fac_SearchI2c(CFactory * self, uint16_t address)
{
    CI2c * found_i2c = NULL;
    uint8_t i;
    uint16_t tmp_addr = address;

    if ((tmp_addr != UCS_ADDR_LOCAL_DEV) && (Net_IsOwnAddress(self->net_ptr, tmp_addr) == NET_IS_OWN_ADDR_NODE))
    {
        tmp_addr = UCS_ADDR_LOCAL_DEV;
    }

    for (i = 0U; (i<FAC_NUM_DEVICES) && (!Fac_IsI2cUninitialized(&self->i2c_list[i])); i++)
    {
        if (tmp_addr == Inic_GetTargetAddress(self->i2c_list[i].nsm_ptr->rsm_ptr->inic_ptr))
        {
            found_i2c = &self->i2c_list[i];
            break;
        }
    }

    return found_i2c;
}

/*! \brief Returns the next free uninitialized XRM instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized XRM instance if found, otherwise \c NULL.
 */
static CExtendedResourceManager * Fac_GetUninitializedXrm (CFactory * self)
{
    CExtendedResourceManager * tmp_xrm = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (self->xrm_list[i].rsm_ptr == NULL)
        {
            tmp_xrm = &self->xrm_list[i];
            break;
        }
    }

    return tmp_xrm;
}

/*! \brief Returns the next free uninitialized FBI instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized FBI instance if found, otherwise \c NULL.
 */
static CInic * Fac_GetUninitializedFbi (CFactory * self)
{
    CInic * tmp_inic = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (self->fbi_list[i].base_ptr == NULL)
        {
            tmp_inic = &self->fbi_list[i];
            break;
        }
    }

    return tmp_inic;
}

/*! \brief Returns the next free uninitialized NSM instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized NSM instance if found, otherwise \c NULL.
 */
static CNodeScriptManagement * Fac_GetUninitializedNsm (CFactory * self)
{
    CNodeScriptManagement * tmp_nsm = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (self->nsm_list[i].base_ptr == NULL)
        {
            tmp_nsm = &self->nsm_list[i];
            break;
        }
    }

    return tmp_nsm;
}

/*! \brief Returns the next free uninitialized RSM instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized RSM instance if found, otherwise \c NULL.
 */
static CRemoteSyncManagement * Fac_GetUninitializedRsm (CFactory * self)
{
    CRemoteSyncManagement * tmp_rsm = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (Inic_GetTargetAddress(self->rsm_list[i].inic_ptr) == 0x0U)
        {
            tmp_rsm = &self->rsm_list[i];
            break;
        }
    }

    return tmp_rsm;
}

/*! \brief Returns the next free uninitialized GPIO instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized GPIO instance if found, otherwise \c NULL.
 */
static CGpio * Fac_GetUninitializedGpio (CFactory * self)
{
    CGpio * tmp_gpio = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (NULL == self->gpio_list[i].nsm_ptr)
        {
            tmp_gpio = &self->gpio_list[i];
            break;
        }
    }

    return tmp_gpio;
}

/*! \brief Returns the next free uninitialized I2C instance
 *  \param self    Instance pointer
 *  \return a reference to the next free uninitialized I2C instance if found, otherwise \c NULL.
 */
static CI2c * Fac_GetUninitializedI2c (CFactory * self)
{
    CI2c * tmp_i2c = NULL;
    uint8_t i;

    for (i = 0U; i<FAC_NUM_DEVICES; i++)
    {
        if (NULL == self->i2c_list[i].nsm_ptr)
        {
            tmp_i2c = &self->i2c_list[i];
            break;
        }
    }

    return tmp_i2c;
}

/*! \brief Constructs the given FBI instance
 *  \param self       the UCS factory Instance pointer
 *  \param fbi        the INIC Instance pointer
 *  \param address    the device address of this FBlock INIC
 */
static void Fac_ConstructFbi (CFactory * self, CInic * fbi, uint16_t address)
{
    Inic_InitData_t inic_init_data;

    if (address == UCS_ADDR_LOCAL_DEV)
    {
        inic_init_data.xcvr_ptr = self->icm_transceiver;
    }
    else
    {
        inic_init_data.xcvr_ptr = self->rcm_transceiver;
    }

    inic_init_data.base_ptr = self->base_ptr;
    inic_init_data.tgt_addr = address;

    Inic_Ctor(fbi, &inic_init_data);
}

/*! \brief Constructs the given NSM instance
 *  \param self       the UCS factory Instance pointer
 *  \param nsm_ptr    the NSM Instance pointer
 *  \param rsm_ptr    the RSM Instance pointer
 */
static void Fac_ConstructNsm (CFactory * self, CNodeScriptManagement * nsm_ptr, CRemoteSyncManagement * rsm_ptr)
{
    Nsm_InitData_t nsm_init_data;

    nsm_init_data.base_ptr = self->base_ptr;
    nsm_init_data.rcm_ptr  = self->rcm_transceiver;
    nsm_init_data.rsm_ptr  = rsm_ptr;

    Nsm_Ctor(nsm_ptr, &nsm_init_data);
}

/*! \brief Checks whether the given FBlock INIC instance is uninitialized
 *  \param fbi        the INIC Instance pointer
 *  \return \c true if the given Fbi instance is not initialized, otherwise \c False.
 */
static bool Fac_IsFbiUninitialized(CInic * fbi)
{
    return (fbi->base_ptr == NULL) ;
}

/*! \brief Checks whether the given NSM instance is uninitialized
 *  \param nsm        the NSM Instance pointer
 *  \return \c true if the given NSM instance is not initialized, otherwise \c False.
 */
static bool Fac_IsNsmUninitialized(CNodeScriptManagement * nsm)
{
    return (nsm->base_ptr == NULL) ;
}

/*! \brief Checks whether the given RSM instance is uninitialized
 *  \param rsm   Reference to the RSM instance pointer
 *  \return \c true if the given Fbi instance is not initialized, otherwise \c False.
 */
static bool Fac_IsRsmUninitialized(CRemoteSyncManagement * rsm)
{
    return Fac_IsFbiUninitialized(rsm->inic_ptr);
}

/*! \brief Checks whether the given XRM instance is uninitialized
 *  \param xrm        the XRM Instance pointer
 *  \return \c true if the given XRM instance is not initialized, otherwise \c False.
 */
static bool Fac_IsXrmUninitialized(CExtendedResourceManager * xrm)
{
    return (xrm->rsm_ptr == NULL) ;
}

/*! \brief Checks whether the given GPIO instance is uninitialized
 *  \param gpio        the GPIO Instance pointer
 *  \return \c true if the given GPIO instance is not initialized, otherwise \c False.
 */
static bool Fac_IsGpioUninitialized(CGpio * gpio)
{
    return (NULL == gpio->nsm_ptr);
}

/*! \brief Checks whether the given I2C instance is uninitialized
 *  \param i2c        the I2C Instance pointer
 *  \return \c true if the given I2C instance is not initialized, otherwise \c False.
 */
static bool Fac_IsI2cUninitialized(CI2c * i2c)
{
    return (NULL == i2c->nsm_ptr);
}

/*!
 * @}
 * \endcond
 */

/*------------------------------------------------------------------------------------------------*/
/* End of file                                                                                    */
/*------------------------------------------------------------------------------------------------*/

