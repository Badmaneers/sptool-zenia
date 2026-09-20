#ifndef DASLACALLBACK
#define DASLACALLBACK

#include "../../BootRom/flashtool_api.h"

class Connection;

class DASLACB
{
public:
    static BOOL __stdcall SERVER_VERIFICATION_FLOW(struct ft_sec_flow_parameters *param);
    static BOOL __stdcall SERVER_SIGNATURE_DATA(char* pbuf, unsigned int max_buf_length, unsigned int* length);

    static void __stdcall setFlashToolAPIHandle(FLASHTOOL_API_HANDLE_T ft);

private:
    static BOOL __stdcall checkDASLAEnabled();
    static void __stdcall setDefaultPolicy(struct ft_sec_flow_parameters *param);

private:
    static FLASHTOOL_API_HANDLE_T m_ft;
};

#endif // DASLACALLBACK

