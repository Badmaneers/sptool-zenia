#include "SetResetBootModeCommand.h"
#include "../Conn/Connection.h"
#include "../BootRom/flashtoolex_api.h"
#include "../Logger/Log.h"
#include "../Err/Exception.h"

namespace APCore
{

SetResetBootModeCommand::SetResetBootModeCommand(APKey key):
    ICommand(key)
{
}

SetResetBootModeCommand::~SetResetBootModeCommand()
{
}

void SetResetBootModeCommand::exec(const QSharedPointer<APCore::Connection> &conn)
{
    conn->ConnectDA();

    unsigned int n_BM_ATM = BM_ATM;

    int ret = FlashTool_Device_Control(conn->FTHandle(), DEV_SET_BOOT_MODE, (pvoid)&n_BM_ATM, sizeof(n_BM_ATM), 0, 0, 0);
    if(ret != STATUS_OK)
    {
        LOGE("FlashTool_Device_Control::DEV_SET_BOOT_MODE failed! ret: %s(0x%x)", StatusToString(ret), ret);
        THROW_BROM_EXCEPTION(ret, 0);
    }
    LOGI("FlashTool_Device_Control::DEV_SET_BOOT_MODE Succeeded.");
}


}
