#include "ConsoleModeCallback.h"
#include <QString>
#include <QStringList>
#include "../Logger/Log.h"
#include "../Network/NetworkThread.h"
#include "ConsoleModeEntry.h"
#include "../Utility/version.h"

namespace ConsoleMode
{

ConsoleModeEntry *ConsoleModeCallback::m_console_mode_entry = NULL;

void ConsoleModeCallback::set_console_mode_entry(ConsoleModeEntry *entry)
{
    m_console_mode_entry = entry;
}

void ConsoleModeCallback::query_device_tracking(int *p_out)
{
    Q_ASSERT(NULL != p_out);
/*
#define MTK_VID "vid_0e8d"

    *p_out = 1;

    QString vid = QString::fromStdString(pid_vid).split("&").at(0);

    if (vid.toLower() != MTK_VID) {
        *p_out = 0;
        LOG("mismatch vendor id: %s", vid.toStdString().c_str());
    }
*/
    *p_out = 0;
}

void ConsoleModeCallback::da_onnected(const DA_REPORT_T &da_report, const std::string &friendly_name,
                                      const int usb_status, bool need_collect_dev_info)
{
#ifdef _INTERNAL_PHONE_TRACKING_MODE
    Q_UNUSED(friendly_name)
    Q_UNUSED(usb_status)

    if(ToolInfo::IsCustomerVer()) {
        LOGI("No need collect device tracking information for customer version!");
        return ;
    }

    if (!need_collect_dev_info) {
        LOGI("No need collect device tracking information due to NOT support this feature on this platform!");
        return ;
    }

    if (!m_console_mode_entry->network_online()) {
        LOGI("No need collect device tracking information due to NO network!");
        return ;
    }

    QString device_id;
    if (da_report.m_emmc_ret == S_DONE)
    {
#define MMC_CID_CNT sizeof(da_report.m_emmc_cid) / sizeof(da_report.m_emmc_cid[0])
        for (int i = 0; i < MMC_CID_CNT; ++i) {
            U32 cid = Utils::xhtoni(da_report.m_emmc_cid[i]);
            device_id.append(QString("%1").arg(cid, 0, 16));
        }
        //device_id = QString("%1%2%3%4").arg(da_report.m_emmc_cid[0], 0, 16).arg(da_report.m_emmc_cid[1], 0, 16)
                //.arg(da_report.m_emmc_cid[2], 0, 16).arg(da_report.m_emmc_cid[3], 0, 16);
    }
    else if (da_report.m_ufs_ret == S_DONE)
    {
        device_id = QString::fromStdString((char *)da_report.m_ufs_sn);
    }
    if (!device_id.isEmpty()) {
        NetworkThread *network_thread = new NetworkThread(device_id);
        QObject::connect(network_thread, SIGNAL(finished()), network_thread, SLOT(deleteLater()));
        network_thread->start();
    }
#endif
}

}
