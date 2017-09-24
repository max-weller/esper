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

    debug_d("Device: " DEVICE);
    debug_d("SDK Version: %s", system_get_sdk_version());
    debug_d("Boot Version: %s", system_get_boot_version());
    debug_d("Boot Mode: %d", system_get_boot_mode());
    debug_d("ESPer Version: " VERSION);
    debug_d("Free Heap: %d", system_get_free_heap_size());
    debug_d("CPU Frequency: %d MHz", system_get_cpu_freq());
    debug_d("System Chip ID: 0x%06x", system_get_chip_id());
    debug_d("SPI Flash ID: 0x%06x", spi_flash_get_id());
    debug_d("ROM Selected: %d", rbootconf.current_rom);
    debug_d("ROM Slot 0: 0x%08x", rbootconf.roms[0]);
    debug_d("ROM Slot 1: 0x%08x", rbootconf.roms[1]);

#ifdef INFO_HTTP_ENABLED
    http.listen(INFO_HTTP_PORT);
	http.addPath("/", HttpPathDelegate(&Info::onHttpIndex, this));
#endif
}

Info::~Info() {
}

void Info::onStateChanged(const State& state) {
    switch (state) {
        case State::CONNECTED: {
            // Record time when going online
            this->connectTime = RTC.getRtcSeconds();

            this->publish();
            this->timer.start();

            break;
        }

        case State::DISCONNECTED: {
            this->timer.stop();

            break;
        }
    }
}

void Info::publish() {
    this->device->publish(Device::TOPIC_BASE + "/$localip", WifiStation.getIP().toString(), true);
    this->device->publish(Device::TOPIC_BASE + "/$mac", WifiStation.getMAC(), true);
    this->device->publish(Device::TOPIC_BASE + "/$fw/name", DEVICE, true);
    this->device->publish(Device::TOPIC_BASE + "/$fw/version", VERSION);
    this->device->publish(Device::TOPIC_BASE + "/$impl/sdk_version", system_get_sdk_version());
    this->device->publish(Device::TOPIC_BASE + "/$impl/boot_version", system_get_boot_version());
    this->device->publish(Device::TOPIC_BASE + "/$impl/chip_id", String(system_get_chip_id(), 16), true);
    this->device->publish(Device::TOPIC_BASE + "/$impl/flash_id", String(spi_flash_get_id(), 16), true);

    this->device->publish(Device::TOPIC_BASE + "/$impl/boot_rom", rboot_get_current_rom());
    this->device->publish(Device::TOPIC_BASE + "/$impl/startup_time", this->startupTime);
    this->device->publish(Device::TOPIC_BASE + "/$impl/connect_time", this->connectTime);
    this->device->publish(Device::TOPIC_BASE + "/$impl/updated_time", RTC.getRtcSeconds());

    this->device->publish(Device::TOPIC_BASE + "/$impl/wifi_ssid", this->device->getWifi().getCurrentSSID());
    this->device->publish(Device::TOPIC_BASE + "/$impl/wifi_bssid", this->device->getWifi().getCurrentBSSID());
    this->device->publish(Device::TOPIC_BASE + "/$stats/signal", WifiStation.getRssi());
    this->device->publish(Device::TOPIC_BASE + "/$impl/wifi_channel", WifiStation.getChannel());

    String nodes;
    for (int i = 0; i < this->device->getServices().count(); i++) {
        if (i > 0) nodes += ",";
        nodes += this->device->getServices().at(i)->getName();
    }
    this->device->publish(Device::TOPIC_BASE + "/$nodes", nodes);
}

#ifdef INFO_HTTP_ENABLED
void Info::onHttpIndex(HttpRequest &request, HttpResponse &response) {
    response.setContentType("application/json");
    response.sendString(this->dump());
}
#endif
