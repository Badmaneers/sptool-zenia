#ifndef SETRESETBOOTMODECOMMAND_H
#define SETRESETBOOTMODECOMMAND_H

#include "ICommand.h"

namespace APCore
{

class SetResetBootModeCommand : public ICommand
{
public:
    SetResetBootModeCommand(APKey key);
    virtual ~SetResetBootModeCommand();

    virtual void exec(const QSharedPointer<APCore::Connection> &conn);
};

}

#endif // SETRESETBOOTMODECOMMAND_H
