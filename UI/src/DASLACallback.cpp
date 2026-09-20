#include "DASLACallback.h"
#include <QtGlobal>
#include <QLibrary>
#include <QVector>
#include "../../BootRom/SLA_Challenge.h"
#include "../../Logger/Log.h"
#include "../../BootRom/common_struct.h"
#include "../../Conn/Connection.h"

typedef int (__stdcall *CB_SLA_CHALLENGE_CONFIG)();
typedef int (__stdcall *CB_DA_SLA_Challenge)(void *usr_arg, const
                                             unsigned char  *p_challenge_in,
                                             unsigned int  challenge_in_len,
                                             unsigned char  **pp_challenge_out,
                                             unsigned int  *p_challenge_out_len);
typedef int (__stdcall *CB_DA_SLA_Challenge_END)(void *usr_arg, unsigned char  *p_challenge_out);

FLASHTOOL_API_HANDLE_T DASLACB::m_ft = NULL;

BOOL __stdcall DASLACB::SERVER_VERIFICATION_FLOW(struct ft_sec_flow_parameters *param)
{
    sla_challenge_in_data_t *sla_challenge_in_data = (sla_challenge_in_data_t *)(param->fw_info.data);
    static QVector<uint32> valid_version_list;
    valid_version_list.push_back(0x10000);
    if (!valid_version_list.contains(sla_challenge_in_data->version)) {
        setDefaultPolicy(param);
        LOGI("Warning: not valid DA SLA version, skip DA SLA flow.");
        return TRUE;
    }

    if (!checkDASLAEnabled()) {
        setDefaultPolicy(param);
        LOGW("Warning: DA SLA Disabled, skip it!");
        return TRUE;
    }

    QString library_name("SLA_Challenge");
#ifndef _WIN32
    library_name = library_name.toLower();
#endif
    sla_feature_e sla_feature = FEATURE_NONE;
    CB_SLA_CHALLENGE_CONFIG sla_feature_func = (CB_SLA_CHALLENGE_CONFIG)QLibrary::resolve(library_name, "SLA_Feature_Config");
    if (sla_feature_func) {
        sla_feature = (sla_feature_e)sla_feature_func();
    } else {
        LOGI("NO SLA API SLA_Feature_Config.");
    }

    unsigned int challenge_in_data_offset = (sla_feature == FEATURE_HRID) ? SLA_HRID_LENGTH :
                                            (sla_feature == FEATURE_SOCID) ? SLA_SOCID_LENGTH : 0;
    unsigned int challenge_in_data_len = SLA_RANDOM_LENGTH + challenge_in_data_offset;
    unsigned char *p_challenge_in_data = (unsigned char *)malloc(challenge_in_data_len);
    if (NULL == p_challenge_in_data) {
        LOGE("insufficient memory for sla data.");
        return FALSE;
    }
    memset(p_challenge_in_data, 0, challenge_in_data_len);
    if (sla_feature == FEATURE_HRID) {
        memcpy(p_challenge_in_data, sla_challenge_in_data->hrid, challenge_in_data_offset);
    } else if (sla_feature == FEATURE_SOCID) {
        memcpy(p_challenge_in_data, sla_challenge_in_data->soc_id, challenge_in_data_offset);
    }
    memcpy(p_challenge_in_data + challenge_in_data_offset, sla_challenge_in_data->random_data, SLA_RANDOM_LENGTH);

    memset(&param->sec_policy, 0, sizeof(param->sec_policy));
    unsigned char *pPolicyData = NULL;
    CB_DA_SLA_Challenge da_sla_challenge_func = (CB_DA_SLA_Challenge)QLibrary::resolve(library_name, "DA_SLA_Challenge");
    if (!da_sla_challenge_func) {
        LOGE("NO SLA API DA_SLA_Challenge.");
        return FALSE;
    }
    int status = da_sla_challenge_func(NULL, p_challenge_in_data, challenge_in_data_len,
                               &pPolicyData, &param->sec_policy.length);
    free(p_challenge_in_data);
    if(0 != status)
    {
        LOGI("Warning: SLA_Challenge failed, error code: %d", status);
        setDefaultPolicy(param);
    } else {
        memcpy(param->sec_policy.data, pPolicyData, param->sec_policy.length);
    }
    CB_DA_SLA_Challenge_END da_sla_challenge_end_func = (CB_DA_SLA_Challenge_END)QLibrary::resolve(library_name, "DA_SLA_Challenge_END");
    if (da_sla_challenge_end_func) {
        da_sla_challenge_end_func(NULL, 0);
    } else {
        LOGE("NO SLA API DA_SLA_Challenge_END");
        return FALSE;
    }
    return TRUE;
}

BOOL __stdcall DASLACB::SERVER_SIGNATURE_DATA(char* pbuf, unsigned int max_buf_length, unsigned int* length)
{
    // NULL implementation, and it nothing to do with DA SLA Feature.
    Q_UNUSED(pbuf)
    Q_UNUSED(max_buf_length)
    Q_UNUSED(length)
    return TRUE;
}

void DASLACB::setFlashToolAPIHandle(FLASHTOOL_API_HANDLE_T ft)
{
    m_ft = ft;
}

BOOL __stdcall DASLACB::checkDASLAEnabled()
{
    if (NULL == m_ft) {
        LOGW("DA SLA enabled check skipped due to no valid FlashToolAPIHandler_T pointer!");
        return FALSE;
    }

    int da_sla_enabled = 0;
    unsigned int returned_byte = 0;
    int ret = FlashTool_Device_Control(m_ft, DEV_DA_GET_SLA_ENABLED_STATUS, 0, 0, (void *)&da_sla_enabled, sizeof(int), &returned_byte);
    if (STATUS_OK != ret)
    {
        LOGE("DA SLA enabled check failed: %s(%d)", StatusToString(ret), ret);
        return FALSE;
    }
    LOGI("DA SLA enabled status: %s.", da_sla_enabled ? "Enabled" : "Disabled");
    return da_sla_enabled;
}

void DASLACB::setDefaultPolicy(ft_sec_flow_parameters *param)
{
    memcpy(param->sec_policy.data, "SLA", 4);
    param->sec_policy.length = 4;
}
