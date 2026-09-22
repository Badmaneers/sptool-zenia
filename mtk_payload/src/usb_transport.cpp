#include "usb_transport.h"
#include "utils.h"
#include "logger.h"
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

namespace mtk {

UsbTransport::UsbTransport() {
    libusb_init(&ctx_);
}

UsbTransport::~UsbTransport() {
    close(false);
    if (ctx_) {
        libusb_exit(ctx_);
        ctx_ = nullptr;
    }
}

bool UsbTransport::connect(uint16_t vid, uint16_t pid, int /*interface_num*/) {
    if (connected_) {
        close(false);
    }

    vid_ = vid;
    pid_ = pid;

    handle_ = libusb_open_device_with_vid_pid(ctx_, vid, pid);
    if (!handle_) {
        LOG_DEBUG("Couldn't detect device %04X:%04X", vid, pid);
        return false;
    }

    // Found device, set up endpoints
    libusb_device* dev = libusb_get_device(handle_);
    struct libusb_config_descriptor* config;
    if (libusb_get_active_config_descriptor(dev, &config) < 0) {
        LOG_ERROR("Couldn't get config descriptor");
        release_resources();
        return false;
    }

    // Find CDC data interface
    interface_ = -1;
    for (int i = 0; i < config->bNumInterfaces; ++i) {
        const struct libusb_interface* iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; ++j) {
            const struct libusb_interface_descriptor* alt = &iface->altsetting[j];
            if (alt->bInterfaceClass == 10) { // CDC Data
                interface_ = alt->bInterfaceNumber;
                for (int k = 0; k < alt->bNumEndpoints; ++k) {
                    const struct libusb_endpoint_descriptor* ep = &alt->endpoint[k];
                    if (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                        if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                            ep_in_ = ep->bEndpointAddress;
                            max_packet_size_ = ep->wMaxPacketSize;
                        } else {
                            ep_out_ = ep->bEndpointAddress;
                        }
                    }
                }
                break;
            }
        }
        if (interface_ != -1) break;
    }

    libusb_free_config_descriptor(config);

    if (interface_ == -1) {
        LOG_ERROR("Couldn't find CDC interface");
        release_resources();
        return false;
    }

    // Detach kernel driver if active (matches Python behavior)
    try {
        if (libusb_kernel_driver_active(handle_, 0) == 1) {
            LOG_DEBUG("Detaching kernel driver on interface 0");
            libusb_detach_kernel_driver(handle_, 0);
        }
    } catch (...) {}

    if (libusb_claim_interface(handle_, 0) < 0) {
        LOG_ERROR("Couldn't claim interface 0");
        release_resources();
        return false;
    }

    // Detach kernel driver on data interface too (matches Python)
    try {
        if (libusb_kernel_driver_active(handle_, interface_) == 1) {
            LOG_DEBUG("Detaching kernel driver on interface %d", interface_);
            libusb_detach_kernel_driver(handle_, interface_);
        }
    } catch (...) {}

    if (interface_ != 0) {
        try {
            libusb_claim_interface(handle_, interface_);
        } catch (...) {}
    }

    connected_ = true;
    LOG_DEBUG("Connected to USB device %04X:%04X on interface %d", vid, pid, interface_);
    LOG_DEBUG("EP_IN=0x%02X EP_OUT=0x%02X max_packet=%zu", ep_in_, ep_out_, max_packet_size_);
    return true;
}

bool UsbTransport::connect(std::map<uint16_t, std::vector<uint16_t>> portconfig, int devclass) {
    if (connected_) {
        close(false);
    }

    // Enumerate all USB devices (matches Python: usb.core.find(find_all=True))
    libusb_device** dev_list;
    ssize_t cnt = libusb_get_device_list(ctx_, &dev_list);
    if (cnt < 0) {
        LOG_ERROR("Failed to enumerate USB devices");
        return false;
    }

    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device* dev = dev_list[i];
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) < 0) continue;

        // Check if VID is in our supported list
        auto vid_it = portconfig.find(desc.idVendor);
        if (vid_it == portconfig.end()) continue;

        // Check if PID is in the list for this VID
        const auto& pids = vid_it->second;
        if (std::find(pids.begin(), pids.end(), desc.idProduct) == pids.end()) continue;

        // Check device class
        if (devclass != -1) {
            struct libusb_config_descriptor* config;
            if (libusb_get_active_config_descriptor(dev, &config) < 0) continue;

            bool found_class = false;
            for (int j = 0; j < config->bNumInterfaces; ++j) {
                const struct libusb_interface* iface = &config->interface[j];
                for (int k = 0; k < iface->num_altsetting; ++k) {
                    if (iface->altsetting[k].bInterfaceClass == devclass) {
                        found_class = true;
                        break;
                    }
                }
                if (found_class) break;
            }
            libusb_free_config_descriptor(config);
            if (!found_class) continue;
        }

        // Found matching device
        vid_ = desc.idVendor;
        pid_ = desc.idProduct;

        if (libusb_open(dev, &handle_) < 0) {
            LOG_DEBUG("Failed to open device %04X:%04X", vid_, pid_);
            continue;
        }

        // Find CDC data interface
        struct libusb_config_descriptor* config;
        if (libusb_get_active_config_descriptor(dev, &config) < 0) {
            libusb_close(handle_);
            handle_ = nullptr;
            continue;
        }

        interface_ = -1;
        for (int j = 0; j < config->bNumInterfaces; ++j) {
            const struct libusb_interface* iface = &config->interface[j];
            for (int k = 0; k < iface->num_altsetting; ++k) {
                const struct libusb_interface_descriptor* alt = &iface->altsetting[k];
                if (alt->bInterfaceClass == devclass) {
                    interface_ = alt->bInterfaceNumber;
                    for (int l = 0; l < alt->bNumEndpoints; ++l) {
                        const struct libusb_endpoint_descriptor* ep = &alt->endpoint[l];
                        if (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                            if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                                ep_in_ = ep->bEndpointAddress;
                                max_packet_size_ = ep->wMaxPacketSize;
                            } else {
                                ep_out_ = ep->bEndpointAddress;
                            }
                        }
                    }
                    break;
                }
            }
            if (interface_ != -1) break;
        }

        libusb_free_config_descriptor(config);

        if (interface_ == -1) {
            libusb_close(handle_);
            handle_ = nullptr;
            continue;
        }

        // Detach kernel driver if active
        try {
            if (libusb_kernel_driver_active(handle_, 0) == 1) {
                libusb_detach_kernel_driver(handle_, 0);
            }
        } catch (...) {}

        if (libusb_claim_interface(handle_, 0) < 0) {
            libusb_close(handle_);
            handle_ = nullptr;
            continue;
        }

        // Detach and claim data interface
        try {
            if (libusb_kernel_driver_active(handle_, interface_) == 1) {
                libusb_detach_kernel_driver(handle_, interface_);
            }
        } catch (...) {}

        if (interface_ != 0) {
            try {
                libusb_claim_interface(handle_, interface_);
            } catch (...) {}
        }

        connected_ = true;
        libusb_free_device_list(dev_list, 1);
        LOG_DEBUG("Connected to USB device %04X:%04X on interface %d", vid_, pid_, interface_);
        LOG_DEBUG("EP_IN=0x%02X EP_OUT=0x%02X max_packet=%zu", ep_in_, ep_out_, max_packet_size_);
        return true;
    }

    libusb_free_device_list(dev_list, 1);
    LOG_DEBUG("Couldn't detect device. Is it connected?");
    return false;
}

void UsbTransport::close(bool reset) {
    if (handle_) {
        if (reset) {
            libusb_reset_device(handle_);
        }
        // Reattach kernel driver on close (matches Python behavior)
        try {
            if (!libusb_kernel_driver_active(handle_, interface_)) {
                libusb_attach_kernel_driver(handle_, interface_);
            }
        } catch (...) {}
        try {
            if (!libusb_kernel_driver_active(handle_, 0)) {
                libusb_attach_kernel_driver(handle_, 0);
            }
        } catch (...) {}
        libusb_release_interface(handle_, interface_);
        libusb_close(handle_);
        handle_ = nullptr;
    }
    connected_ = false;
    ep_in_ = 0;
    ep_out_ = 0;

    if (reset) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

bool UsbTransport::is_connected() const {
    return connected_;
}

bool UsbTransport::write(const uint8_t* data, size_t len) {
    if (!connected_ || !handle_) return false;
    if (len == 0) {
        int transferred = 0;
        int ret = libusb_bulk_transfer(handle_, ep_out_, nullptr, 0,
                                       &transferred, 1000);
        return ret == LIBUSB_SUCCESS || ret == LIBUSB_ERROR_TIMEOUT;
    }

    size_t pos = 0;
    while (pos < len) {
        int transferred = 0;
        size_t chunk = std::min(len - pos, max_packet_size_);
        int ret = libusb_bulk_transfer(handle_, ep_out_,
                                       const_cast<uint8_t*>(data + pos),
                                       static_cast<int>(chunk),
                                       &transferred, 1000);
        if (ret < 0 && ret != LIBUSB_ERROR_TIMEOUT) {
            LOG_ERROR("USB write error: %s", libusb_error_name(ret));
            return false;
        }
        pos += transferred;
    }
    return true;
}

bool UsbTransport::usbwrite(const uint8_t* data, size_t len) {
    return write(data, len);
}

std::vector<uint8_t> UsbTransport::read(size_t len, int timeout_ms) {
    if (!connected_ || !handle_) return {};

    std::vector<uint8_t> result;
    result.reserve(len);

    while (result.size() < len) {
        size_t to_read = std::min(len - result.size(), max_packet_size_);
        std::vector<uint8_t> buf(to_read);
        int transferred = 0;
        int ret = libusb_bulk_transfer(handle_, ep_in_,
                                       buf.data(), static_cast<int>(to_read),
                                       &transferred, timeout_ms);
        if (ret == LIBUSB_ERROR_TIMEOUT) {
            break;
        }
        if (ret == LIBUSB_ERROR_OVERFLOW) {
            // Device sent more than requested — data in buf is valid, just truncated
            // Don't log error, this is normal during exploit
            if (transferred > 0) {
                size_t avail = std::min(static_cast<size_t>(transferred), len - result.size());
                result.insert(result.end(), buf.begin(), buf.begin() + avail);
            }
            break;
        }
        if (ret < 0) {
            LOG_ERROR("USB read error: %s", libusb_error_name(ret));
            break;
        }
        if (transferred > 0) {
            result.insert(result.end(), buf.begin(), buf.begin() + transferred);
        }
    }

    return result;
}

std::vector<uint8_t> UsbTransport::usbread(size_t len, int timeout_ms) {
    return read(len, timeout_ms);
}

int UsbTransport::ctrl_transfer(uint8_t bm_request_type, uint8_t b_request,
                                uint16_t w_value, uint16_t w_index,
                                uint8_t* data, uint16_t w_length) {
    if (!handle_) return -1;

    int ret = libusb_control_transfer(handle_,
                                      bm_request_type,
                                      b_request,
                                      w_value,
                                      w_index,
                                      data,
                                      w_length,
                                      1000);
    return ret;
}

std::vector<uint8_t> UsbTransport::ctrl_transfer_read(
    uint8_t bm_request_type, uint8_t b_request,
    uint16_t w_value, uint16_t w_index, uint16_t w_length) {
    std::vector<uint8_t> data(w_length);
    int ret = ctrl_transfer(bm_request_type, b_request, w_value, w_index,
                            data.data(), w_length);
    if (ret < 0) return {};
    data.resize(ret);
    return data;
}

uint32_t UsbTransport::rdword(int count) {
    auto data = usbread(4 * count);
    if (data.size() < static_cast<size_t>(4 * count)) return 0;
    return get_be32(data.data());
}

uint16_t UsbTransport::rword(int count) {
    auto data = usbread(2 * count);
    if (data.size() < static_cast<size_t>(2 * count)) return 0;
    return get_be16(data.data());
}

std::vector<uint8_t> UsbTransport::rbyte(int count) {
    return usbread(count);
}

void UsbTransport::set_line_coding(uint32_t baudrate, uint8_t parity,
                                   uint8_t databits, uint8_t stopbits) {
    if (!handle_) return;

    uint8_t linecode[7];
    linecode[0] = static_cast<uint8_t>(baudrate & 0xFF);
    linecode[1] = static_cast<uint8_t>((baudrate >> 8) & 0xFF);
    linecode[2] = static_cast<uint8_t>((baudrate >> 16) & 0xFF);
    linecode[3] = static_cast<uint8_t>((baudrate >> 24) & 0xFF);
    linecode[4] = stopbits; // 0=1, 1=1.5, 2=2
    linecode[5] = parity;   // 0=none, 1=odd, 2=even
    linecode[6] = databits;

    // bmRequestType: 0x21 = OUT | Class | Interface
    ctrl_transfer(0x21, 0x20, 0, 0, linecode, sizeof(linecode));
}

void UsbTransport::set_control_line_state(bool rts, bool dtr) {
    if (!handle_) return;

    uint16_t ctrlstate = (rts ? 2 : 0) + (dtr ? 1 : 0);
    ctrl_transfer(0x21, 0x22, ctrlstate, 0, nullptr, 0);
}

void UsbTransport::release_resources() {
    if (handle_) {
        libusb_close(handle_);
        handle_ = nullptr;
    }
    connected_ = false;
}

} // namespace mtk
