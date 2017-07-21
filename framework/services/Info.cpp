#include "Info.h"
#include "../Device.h"


const char INFO_NAME[] = "info";

Info::Info(Device* const device)
        : Service(device) {
    // Publish status regularly
    this->timer.initializeMs(60000, TimerDelegate(&Info::publish, this));

    // Record time then starting up
    this->startupTime = RTC.getRtcSeconds();

    // Read bootloader config
    const rboot_config rbootconf = rboot_get_config();

    debug_i("");
    debug_i("");
    debug_i("Device: %s", DEVICE);
    debug_i("SDK Version: v%s", system_get_sdk_version());
    debug_i("Boot: v%u (%u)", system_get_boot_version(), system_get_boot_mode());
    debug_i("ESPer Version: v%s", VERSION);
    debug_i("Free Heap: %d", system_get_free_heap_size());
    debug_i("CPU Frequency: %d MHz", system_get_cpu_freq());
    debug_i("System Chip ID: %x", system_get_chip_id());
    debug_i("SPI Flash ID: %x", spi_flash_get_id());
    debug_i("ROM Selected: %d", rbootconf.current_rom);
    debug_i("ROM Slot 0: %08X", rbootconf.roms[0]);
    debug_i("ROM Slot 1: %08X", rbootconf.roms[1]);
    debug_i("");
    debug_i("");

}

Info::~Info() {
}

void Info::onStateChanged(const State& state) {
    switch (state) {
        case State::CONNECTED: {
            // Record time when going online
            this->connectTime = RTC.getRtcSeconds();

            this->timer.start();
            this->publish();

            break;
        }

        case State::DISCONNECTED: {
            this->timer.stop();

            break;
        }
    }
}

void Info::publish() {
    LOG.log("Publishing device info");

    this->device->publish(Device::TOPIC_BASE + "/info", StringSumHelper("") +
                                                        "DEVICE=" + DEVICE + "\n" +
                                                        "ESPER=" + VERSION + "\n" +
                                                        "SDK=" + system_get_sdk_version() + "\n" +
                                                        "BOOT=v" + String(system_get_boot_version()) + "\n" +
                                                        "CHIP=" + String(system_get_chip_id(), 16) + "\n" +
                                                        "FLASH=" + String(spi_flash_get_id(), 16) + "\n" +
                                                        "ROM=" + String(rboot_get_current_rom()) + "\n" +
                                                        "TIME_STARTUP=" + String(this->startupTime) + "\n" +
                                                        "TIME_CONNECT=" + String(this->connectTime) + "\n" +
                                                        "TIME_UPDATED=" + String(RTC.getRtcSeconds()) + "\n",
                          true);
}
