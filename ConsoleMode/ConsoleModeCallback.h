#ifndef CONSOLEMODECALLBACK
#define CONSOLEMODECALLBACK

#include <string>
#include "../BootRom/flashtool_api.h"

namespace ConsoleMode
{

class ConsoleModeEntry;

class ConsoleModeCallback
{
public:
    static void set_console_mode_entry(ConsoleModeEntry *entry);
    static void __stdcall query_device_tracking(int *p_out);
    static void da_onnected(const DA_REPORT_T &da_report, const std::string &friendly_name,
                            const int usb_status, bool need_collect_dev_info);

private:
    static ConsoleModeEntry *m_console_mode_entry;
};

}

#endif // CONSOLEMODECALLBACK

