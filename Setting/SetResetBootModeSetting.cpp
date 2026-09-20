#include "SetResetBootModeSetting.h"
#include "../Cmd/SetResetBootModeCommand.h"
#include "../XMLParser/XMLNode.h"
#include "../Host/Inc/RuntimeMemory.h"

namespace APCore
{

SetResetBootModeSetting::SetResetBootModeSetting()
{

}

QSharedPointer<APCore::ICommand> SetResetBootModeSetting::CreateCommand(APKey key)
{
    QSharedPointer<APCore::SetResetBootModeCommand> cmd(new SetResetBootModeCommand(key));

    return cmd;
}

void SetResetBootModeSetting::LoadXML(const XML::Node &node)
{
    LOG("%0xI64x", &node);
}

void SetResetBootModeSetting::SaveXML(XML::Node &node) const
{
    LOG("%0xI64x", &node);
}

}
