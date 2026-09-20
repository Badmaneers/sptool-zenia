#ifndef CONSOLEMODEENTRY_H
#define CONSOLEMODEENTRY_H

#include <QSharedPointer>
#include "../Public/AppCore.h"
#include "../Utility/LogFilesClean.h"

class NetworkThread;

namespace ConsoleMode
{

class ConsoleModeEntry
{
public:
    ConsoleModeEntry();
    ~ConsoleModeEntry();

    int Run(int argc, char *argv[]);
    bool network_online() const;

private:
    ConsoleModeEntry(const ConsoleModeEntry &rhs);
    ConsoleModeEntry & operator=(const ConsoleModeEntry &rhs);
    void CleanLogFiles(const std::string &log_path, qint64 clean_hours);

private:
    LogCleanThread *m_cleanThread;
    QSharedPointer<NetworkThread> m_network_thread;
};

}

#endif // CONSOLEMODEENTRY_H
