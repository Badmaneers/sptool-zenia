#include "DeviceScan.h"
#include "Logger/Log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <netinet/in.h>
#include <QElapsedTimer>

#include "../../BootRom/brom.h"

#define UEVENT_BUFFER_SIZE 2048

DeviceScan::DeviceScan(USB_DEVICE_INFO* info) :
    usb_info_(info),
    start_time_(0),
    end_time_(0),
    hotplug_sock_(0)
{
}

DeviceScan::~DeviceScan()
{
    if(hotplug_sock_ > 0)
       close(hotplug_sock_);
}

int DeviceScan::init_hotplug_sock()
{
    const int bufferSize = 16 * 1024 * 1024;
    struct timeval timeout = {1, 0};

    struct sockaddr_nl snl;
    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = 0;
    snl.nl_groups = 1;

    int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

    if(s == -1)
    {
        LOGI("create socket error: %s\n", strerror(errno));
        return -1;
    }

    int ret = setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &bufferSize, sizeof(bufferSize));
    if (ret == -1)
    {
        LOGI("set socket receive buffer failed: %s\n", strerror(errno));
        close(s);
        return -1;
    }

    ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
    if (ret == -1)
    {
        LOGI("set socket receive timeout failed: %s\n", strerror(errno));
        close(s);
        return -1;
    }

    int tmp = 1;
    ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
    if (ret == -1)
    {
        LOGI("set socket reuse addr option fail: %s\n", strerror(errno));
        close(s);
        return -1;
    }

    ret = bind(s, (struct sockaddr*)&snl, sizeof(struct sockaddr_nl));

    if(ret < 0)
    {
        LOGI("bind socket error: %s\n", strerror(errno));
        close(s);
        return -1;
    }

    return s;
}

bool DeviceScan::VerifyDeviceInfo(size_t ports_count, const USB_DEVICE_INFO *inst)
{
    for (size_t i=0; i < ports_count; ++i)
    {
        if (usb_info_[i].vid == inst->vid &&
                usb_info_[i].pid == inst->pid )
        {
            start_time_ = time(NULL);
            return true;
        }
    }

    return false;
}

bool DeviceScan::WaitForDeviceReady(const char *path)
{
    int fd = -1;

    LOGI("<%s>: waiting for device ready...\n", path);

    for (int i = 0; i < 100; ++i)
    {
        if ((fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY, 0)) >= 0)
        {
            LOGI("<%s>: ready (attempt %d)\n", path, i);
            close(fd);

            if (-1 == chown(path, 0, 0))
            {
                LOGI("chown failed: %s\n", strerror(errno));
            }

            if (-1 == chmod(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
            {
                LOGI("chmod failed: %s\n", strerror(errno));
            }

            usleep(100000);
            end_time_ = time(NULL);
            return true;
        }
        else
        {
            LOGI("[%d] open %s failed: %s\n", i, path, strerror(errno));
        }
        usleep(100000);
    }

    LOGI("<%s>: not ready after 100 attempts!\n", path);
    return false;
}

bool DeviceScan::USBPathMatch(const char *buf, const char *preferComPort) const
{
    LOGI("source usb_path: %s, search usb com port: %s", buf, preferComPort);
    std::string usb_path = std::string(buf);
    size_t npos = usb_path.rfind(':');
    std::string pre_usb_path = usb_path.substr(0, npos);
    npos = pre_usb_path.rfind('/');
    std::string usb_com_port = pre_usb_path.substr(npos + 1);
    return usb_com_port == std::string(preferComPort);
}

bool DeviceScan::GetDeviceInfo(size_t ports_count,
        const char *buf, size_t buf_size,
        char *portName)
{
    const char *prefix = "add@";
    const char *ttyACM = "/ttyACM";
    const char *sysPrefix = "/sys";
    const char *PID = "idProduct";
    const char *VID = "idVendor";
    const char *removePrefix = "remove@";

    char mybuf[UEVENT_BUFFER_SIZE * 2] = {0};

    char *firstPosition;
    char *lastPosition;

    USB_DEVICE_INFO newdev = { 0, 0 };

    memcpy(mybuf, buf, buf_size);

    firstPosition = strstr(mybuf, prefix);
    lastPosition = strstr(mybuf, ttyACM);
    if(!firstPosition)
        firstPosition = strstr(mybuf, removePrefix);

    if(!firstPosition || !lastPosition)
        return false;

    int prefixLen = strlen(prefix);
    int length = lastPosition - firstPosition - prefixLen;

    if(length <= 0 || length >= UEVENT_BUFFER_SIZE)
        return false;

    char path_without_sys_prefix[UEVENT_BUFFER_SIZE] = {0};
    char path_without_sys_prefix_up_dir[UEVENT_BUFFER_SIZE] = {0};
    char path_with_sys_prefix[UEVENT_BUFFER_SIZE] = {0};
    char path_of_pid[UEVENT_BUFFER_SIZE] = {0};
    char path_of_vid[UEVENT_BUFFER_SIZE] = {0};

    memcpy(path_without_sys_prefix, firstPosition + prefixLen, length);

    char *lastToken = strrchr(path_without_sys_prefix, '/');
    if(!lastToken || strlen(lastToken) <= 1)
        return false;

    int len = strlen(lastToken);
    memcpy(path_without_sys_prefix_up_dir, path_without_sys_prefix, length - len);

    snprintf(path_with_sys_prefix, sizeof(path_with_sys_prefix), "%s%s", sysPrefix, path_without_sys_prefix_up_dir);

    snprintf(path_of_vid, sizeof(path_of_vid), "%s/%s", path_with_sys_prefix, VID);
    snprintf(path_of_pid, sizeof(path_of_pid), "%s/%s", path_with_sys_prefix, PID);

    LOGI("sysfs vid path: %s\n", path_of_vid);
    LOGI("sysfs pid path: %s\n", path_of_pid);

    char tmpbuf[5] = {0};

    int fd = open(path_of_vid, O_RDONLY);
    if (fd < 0)
    {
        LOGI("open VID device failed: %s\n", strerror(errno));
        return false;
    }
    memset(tmpbuf, 0, sizeof(tmpbuf));
    ssize_t readSize = read(fd, tmpbuf, sizeof(tmpbuf) - 1);
    close(fd);
    if (readSize <= 0)
    {
        LOGI("read VID failed\n");
        return false;
    }

    newdev.vid = strtol(tmpbuf, NULL, 16);
    LOGI("device vid = %04x\n", newdev.vid);

    fd = open(path_of_pid, O_RDONLY);
    if (fd < 0)
    {
        LOGI("open PID device failed: %s\n", strerror(errno));
        return false;
    }
    memset(tmpbuf, 0, sizeof(tmpbuf));
    readSize = read(fd, tmpbuf, sizeof(tmpbuf) - 1);
    close(fd);
    if (readSize <= 0)
    {
        LOGI("read PID failed\n");
        return false;
    }

    newdev.pid = strtol(tmpbuf, NULL, 16);
    LOGI("device pid = %04x\n", newdev.pid);

    char portNameTmp[UEVENT_BUFFER_SIZE] = {0};
    char *lastSlash = strrchr(lastPosition, '/');
    if(lastSlash && strlen(lastSlash) > 1)
    {
        snprintf(portNameTmp, sizeof(portNameTmp), "%s", lastSlash);
    }
    else
    {
        snprintf(portNameTmp, sizeof(portNameTmp), "%s", lastPosition);
    }
    snprintf(portName, UEVENT_BUFFER_SIZE + 5, "/dev%s", portNameTmp);
    LOGI("com portName is: %s\n", portName);

    if(VerifyDeviceInfo(ports_count, &newdev))
    {
        if(WaitForDeviceReady(portName))
        {
            double totalT = difftime(end_time_, start_time_);
            LOGI("Total wait time = %f\n", totalT);
            return true;
        }
        else
        {
            LOGI("Device %04x:%04x found but not ready at %s\n",
                 newdev.vid, newdev.pid, portName);
        }
    }

    return false;
}

bool DeviceScan::FindDeviceUSBPort(size_t ports_count, char *portName, int* p_stop_flag, const int& d_time_out)
{
    hotplug_sock_ = init_hotplug_sock();
    if(hotplug_sock_ < 0)
    {
        LOGE("Failed to create hotplug socket\n");
        return false;
    }

    struct timeval tv;
    int ret, recvLen;

    QElapsedTimer time;
    time.start();

    while(BOOT_STOP != (*p_stop_flag))
    {
        char buf[UEVENT_BUFFER_SIZE * 2] = {0};

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(hotplug_sock_, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        ret = select(hotplug_sock_ + 1, &fds, NULL, NULL, &tv);

        if (time.elapsed() > d_time_out)
        {
            LOGE("Timeout(%u ms) for searching USB port!\n", d_time_out);
            close(hotplug_sock_);
            hotplug_sock_ = 0;
            return false;
        }

        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            LOGI("select error: %s\n", strerror(errno));
            continue;
        }

        if(!FD_ISSET(hotplug_sock_, &fds))
            continue;

        recvLen = recv(hotplug_sock_, &buf, sizeof(buf), 0);

        if(recvLen > 0)
        {
            LOGI("uevent: %s\n", buf);

            if(GetDeviceInfo(ports_count, buf, sizeof(buf), portName))
            {
                close(hotplug_sock_);
                hotplug_sock_ = 0;
                return true;
            }
        }
    }
    close(hotplug_sock_);
    hotplug_sock_ = 0;
    return false;
}

bool DeviceScan::FindSpecialDeviceUSBPort(size_t ports_count, const char *sPreferComPort, char *portName, int *p_stop_flag, int d_time_out)
{
    hotplug_sock_ = init_hotplug_sock();
    if(hotplug_sock_ < 0)
    {
        LOGE("Failed to create hotplug socket\n");
        return false;
    }

    struct timeval tv;
    int ret, recvLen;

    QElapsedTimer time;
    time.start();

    while(BOOT_STOP != (*p_stop_flag))
    {
        char buf[UEVENT_BUFFER_SIZE * 2] = {0};

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(hotplug_sock_, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        ret = select(hotplug_sock_ + 1, &fds, NULL, NULL, &tv);

        if (time.elapsed() > d_time_out)
        {
            LOGE("Timeout(%u ms) for searching USB port!\n", d_time_out);
            close(hotplug_sock_);
            hotplug_sock_ = 0;
            return false;
        }

        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            LOGI("select error: %s\n", strerror(errno));
            continue;
        }

        if(!FD_ISSET(hotplug_sock_, &fds))
            continue;

        recvLen = recv(hotplug_sock_, &buf, sizeof(buf), 0);

        if(recvLen > 0)
        {
            LOGI("uevent: %s\n", buf);
            if (!USBPathMatch(buf, sPreferComPort))
            {
                LOGI("skip: %s\n", buf);
                continue;
            }

            if(GetDeviceInfo(ports_count, buf, sizeof(buf), portName))
            {
                close(hotplug_sock_);
                hotplug_sock_ = 0;
                return true;
            }
        }
    }
    close(hotplug_sock_);
    hotplug_sock_ = 0;
    return false;
}
