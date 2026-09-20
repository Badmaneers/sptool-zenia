/*
 * Log.cpp
 *
 *  Created on: Aug 27, 2011
 *      Author: MTK81019
 */

#include "Log.h"
#include "../BootRom/brom.h"

#ifdef USE_QTDEBUG

#include <iostream>
#include "../Utility/Utils.h"

#ifdef _WIN32
static const char kSep= '\\';
#else
static const char kSep= '/';
#endif

void QDebugLogFunc::operator()(const char * format,...)
{
    CriticalSectionSentry lck(Logger::GetLogger().GetCriticalSection());

    std::string log_str(format);
    va_list params;
    static char msg[2048] = {0};
#ifndef _WIN32
    log_str = Utils::ReplaceAllSubstring(log_str, WIN_HEX_FORMAT, LINUX_HEX_FORMAT);
#endif
    va_start( params, format );
    vsnprintf( msg, 2040, log_str.c_str(), params );
    va_end( params );
    std::string whole_name(m_file);
    std::string::size_type index = whole_name.find_last_of(kSep);
    if(index != whole_name.npos)
    {
        m_file += index+1;
    }
#ifdef _WIN32
    qDebug()<<m_func<<"():"<<msg<<"("<<m_file<<","<<m_line<<")";
#endif
    //also write to the log file.
    Logger::GetLogger()<<Logger::GetLogger().ContextInfo()
            <<m_func<<"():"
            <<msg<<"("<<m_file<<","<<m_line<<")"<<std::endl;
}
#endif


void LogOperationMgr::DebugLogsOn()
{
    std::string dll_log;
    Logger::GetLogger().DebugOn();
#ifdef _WIN32
    dll_log = Logger::GetLogger().GetSPFlashToolDumpFileFolder() + "\\BROM_DLL_V5.log";
#else
    dll_log = Logger::GetLogger().GetSPFlashToolDumpFileFolder() + "/BROM_DLL_V5.log";
#endif
    Brom_Debug_SetLogFilename(dll_log.c_str());
    Brom_DebugOn();
    Brom_GetDLLInfo(NULL, NULL, NULL, NULL);
}

void LogOperationMgr::DebugLogsOff()
{
    Logger::GetLogger().DebugOff();
    Brom_DebugOff();
}

void LogOperationMgr::LogSwitch(bool enabled_log)
{
    if(enabled_log) {
        DebugLogsOn();
    } else {
        DebugLogsOff();
    }
}

void LogOperationMgr::ResetLogPath(const std::string &new_log_path, bool need_restart_log)
{
    if (need_restart_log) {
        LogOperationMgr::DebugLogsOff();
    }
    Logger::GetLogger().ResetLogPath(new_log_path);
    if (need_restart_log) {
        LogOperationMgr::DebugLogsOn();
    }
}
