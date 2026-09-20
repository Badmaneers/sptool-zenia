#ifndef CONNOB_H
#define CONNOB_H

#include "../../Conn/Connection.h"
#include "MainController.h"

namespace APCore
{
class ConnOb : public ConnObserver
{
public:
    ConnOb(MainController *control);
    ~ConnOb();

    virtual void OnBromConnected(const BOOT_RESULT &, const std::string &);
    virtual void OnDAConnected(const DA_REPORT_T &, const std::string &, const int, bool need_collect_dev_info);

private:
    MainController *control_;
};

class ConnOb_console: public ConnObserver
{
public:
    virtual void OnBromConnected(const BOOT_RESULT &boot_result, const std::string &friendly_name);
    virtual void OnDAConnected(const DA_REPORT_T &da_report, const std::string &friendly_name,
                               const int usb_status, bool need_collect_dev_info);
};

}

#endif // CONNOB_H
