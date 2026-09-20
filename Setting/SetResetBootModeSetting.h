#ifndef SETRESETBOOTMODESETTING_H
#define SETRESETBOOTMODESETTING_H

#include "ISetting.h"

namespace APCore
{

class SetResetBootModeSetting : public ISetting
{
public:
    SetResetBootModeSetting();
    virtual QSharedPointer<APCore::ICommand> CreateCommand(APKey key);

    virtual void LoadXML(const XML::Node &node);
    virtual void SaveXML(XML::Node &node) const;
};

}

#endif // SETRESETBOOTMODESETTING_H
