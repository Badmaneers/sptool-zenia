#include "connob.h"

#include "../../BootRom/flashtool.h"
#include "../../ConsoleMode/ConsoleModeCallback.h"

namespace APCore
{
ConnOb::ConnOb(MainController *control):
    control_(control)
{
}

ConnOb::~ConnOb()
{

}

void ConnOb::OnBromConnected(const BOOT_RESULT &boot_result,
                             const std::string &friendly_name)
{
    control_->get_brom_result(boot_result, friendly_name);
}

void ConnOb::OnDAConnected(const DA_REPORT_T &da_report,
                           const std::string &friendly_name,
                           const int usb_status, bool need_collect_dev_info)
{
    control_->get_da_report(da_report, friendly_name, usb_status, need_collect_dev_info);
}

void ConnOb_console::OnBromConnected(const BOOT_RESULT &boot_result, const std::string &friendly_name)
{
    Q_UNUSED(boot_result)
    Q_UNUSED(friendly_name)
}

void ConnOb_console::OnDAConnected(const DA_REPORT_T &da_report, const std::string &friendly_name,
                                   const int usb_status, bool need_collect_dev_info)
{
    ConsoleMode::ConsoleModeCallback::da_onnected(da_report, friendly_name, usb_status, need_collect_dev_info);
}

}
