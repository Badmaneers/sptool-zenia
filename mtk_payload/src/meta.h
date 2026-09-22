#pragma once
#include "types.h"
#include "preloader.h"
#include <string>

namespace mtk {

class MetaMode {
public:
    MetaMode(Preloader& preloader, MtkConfig& config);

    bool init(const std::string& metamode, bool display = true);
    bool init_wdg(bool display = true);

private:
    bool handle_vendor_auth();

    Preloader& preloader_;
    MtkConfig& config_;
};

} // namespace mtk
