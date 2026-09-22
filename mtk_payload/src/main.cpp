#include "payload_command.h"
#include "logger.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <cstring>

static void print_usage(const char* prog) {
    std::cerr << "MTK Payload Tool (C++ port)\n"
              << "Usage: " << prog << " payload [options]\n\n"
              << "Options:\n"
              << "  --payload <file>      Payload file (optional)\n"
              << "  --ptype <type>        Payload type: kamakiri, kamakiri2, amonet, hashimoto, carbonara\n"
              << "  --metamode <mode>     META mode: FASTBOOT, FACTFACT, METAMETA, FACTORYM, ADVEMETA, AT+NBOOT\n"
              << "  --vid <vid>           USB Vendor ID (hex)\n"
              << "  --pid <pid>           USB Product ID (hex)\n"
              << "  --loader <file>       Custom DA loader\n"
              << "  --preloader <file>    Preloader file for DRAM config\n"
              << "  --var1 <val>          Kamakiri var1 value (hex)\n"
              << "  --da_addr <addr>      Set a specific DA payload addr (hex)\n"
              << "  --brom_addr <addr>    Set a specific BROM payload addr (hex)\n"
              << "  --wdt <addr>          Set a specific watchdog addr (hex)\n"
              << "  --uart_addr <addr>    Set payload uart_addr value (hex)\n"
              << "  --mode <mode>         Set crash mode (0=dasend1,1=dasend2,2=daread)\n"
              << "  --skipwdt             Skip watchdog init\n"
              << "  --crash               Enforce crash if device is in pl mode\n"
              << "  --iot                 Use special mode for IoT MT6261/2301\n"
              << "  --appid <hexstr>      Use app id (hex string)\n"
              << "  --debugmode           Enable debug logging\n"
              << "  --write_preloader_to_file  Dump preloader to file\n"
              << "  --generatekeys        Derive HW keys\n"
              << "  --socid               Read Soc ID\n"
              << "  --help                Show this help\n";
}

static std::string get_arg(int argc, char* argv[], int& i, const std::string& name) {
    if (i + 1 < argc) {
        return argv[++i];
    }
    std::cerr << "Error: " << name << " requires a value\n";
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Find the command
    int cmd_idx = -1;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "payload" ||
            std::string(argv[i]) == "--help" ||
            std::string(argv[i]) == "-h") {
            cmd_idx = i;
            break;
        }
    }

    if (cmd_idx == -1 || std::string(argv[cmd_idx]) == "--help" ||
        std::string(argv[cmd_idx]) == "-h") {
        print_usage(argv[0]);
        return (cmd_idx == -1) ? 1 : 0;
    }

    // Parse arguments
    std::string payload_file, ptype, metamode, loader, preloader_file, appid;
    std::string da_addr, brom_addr, wdt, uart_addr, crash_mode;
    int vid = -1, pid = -1;
    bool debugmode = false;
    bool skipwdt = false;
    bool crash = false;
    bool iot = false;
    bool write_preloader_to_file = false;
    bool generate_keys = false;
    bool socid = false;
    int var1 = -1;

    for (int i = cmd_idx + 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--payload") {
            payload_file = get_arg(argc, argv, i, "--payload");
        } else if (arg == "--ptype") {
            ptype = get_arg(argc, argv, i, "--ptype");
        } else if (arg == "--metamode") {
            metamode = get_arg(argc, argv, i, "--metamode");
        } else if (arg == "--vid") {
            vid = mtk::getint(get_arg(argc, argv, i, "--vid"));
        } else if (arg == "--pid") {
            pid = mtk::getint(get_arg(argc, argv, i, "--pid"));
        } else if (arg == "--loader") {
            loader = get_arg(argc, argv, i, "--loader");
        } else if (arg == "--preloader") {
            preloader_file = get_arg(argc, argv, i, "--preloader");
        } else if (arg == "--var1") {
            var1 = mtk::getint(get_arg(argc, argv, i, "--var1"));
        } else if (arg == "--da_addr") {
            da_addr = get_arg(argc, argv, i, "--da_addr");
        } else if (arg == "--brom_addr") {
            brom_addr = get_arg(argc, argv, i, "--brom_addr");
        } else if (arg == "--wdt") {
            wdt = get_arg(argc, argv, i, "--wdt");
        } else if (arg == "--uart_addr") {
            uart_addr = get_arg(argc, argv, i, "--uart_addr");
        } else if (arg == "--mode") {
            crash_mode = get_arg(argc, argv, i, "--mode");
        } else if (arg == "--appid") {
            appid = get_arg(argc, argv, i, "--appid");
        } else if (arg == "--skipwdt") {
            skipwdt = true;
        } else if (arg == "--crash") {
            crash = true;
        } else if (arg == "--iot") {
            iot = true;
        } else if (arg == "--debugmode") {
            debugmode = true;
        } else if (arg == "--write_preloader_to_file") {
            write_preloader_to_file = true;
        } else if (arg == "--generatekeys") {
            generate_keys = true;
        } else if (arg == "--socid") {
            socid = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    // Execute
    mtk::PayloadCommand cmd;
    auto& config = cmd.config();

    if (var1 != -1) config.chipconfig.var1 = var1;
    if (!da_addr.empty()) config.chipconfig.da_payload_addr = mtk::getint(da_addr);
    if (!brom_addr.empty()) config.chipconfig.brom_payload_addr = mtk::getint(brom_addr);
    if (!wdt.empty()) config.chipconfig.watchdog = mtk::getint(wdt);
    if (!uart_addr.empty()) config.chipconfig.uart = mtk::getint(uart_addr);
    if (skipwdt) config.skipwdt = true;
    if (crash) config.enforcecrash = true;
    if (iot) config.iot = true;
    if (write_preloader_to_file) config.write_preloader_to_file = true;
    if (generate_keys) config.generate_keys = true;
    if (socid) config.readsocid = true;
    if (!appid.empty()) config.appid = appid;

    bool success = cmd.execute(payload_file, ptype, metamode,
                               vid, pid, loader, preloader_file, debugmode);

    return success ? 0 : 1;
}
