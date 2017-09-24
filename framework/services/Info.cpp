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

String Info::dump() const {
    debug_d("Publishing device info");

    StaticJsonBuffer<1024> json;

    auto& info = json.createObject();
    info.set("device", DEVICE);
    info.set("chip_id", String(system_get_chip_id(), 16));
    info.set("flash_id", String(spi_flash_get_id(), 16));

    auto& version = info.createNestedObject("version");
    version.set("esper", VERSION);
    version.set("sdk", system_get_sdk_version());
    version.set("boot", system_get_boot_version());

    auto& boot = info.createNestedObject("boot");
    boot.set("rom", rboot_get_current_rom());

    auto& time = info.createNestedObject("time");
    time.set("startup", this->startupTime);
    time.set("connect", this->connectTime);
    time.set("updated", RTC.getRtcSeconds());

    auto& network = info.createNestedObject("network");
    network.set("mac", WifiStation.getMAC());
    network.set("ip", WifiStation.getIP().toString());
    network.set("mask", WifiStation.getNetworkMask().toString());
    network.set("gateway", WifiStation.getNetworkGateway().toString());

    auto& wifi = info.createNestedObject("wifi");
    wifi.set("ssid", this->device->getWifi().getCurrentSSID());
    wifi.set("bssid", this->device->getWifi().getCurrentBSSID());
    wifi.set("rssi", WifiStation.getRssi());
    wifi.set("channel", WifiStation.getChannel());

    auto& services = info.createNestedArray("services");
    for (int i = 0; i < this->device->getServices().count(); i++) {
        services.add(this->device->getServices().at(i)->getName());
    }

    auto& endpoints = info.createNestedArray("endpoints");
    for (unsigned int i = 0; i < this->device->getSubscriptions().count(); i++) {
        endpoints.add(this->device->getSubscriptions().keyAt(i));
    }

    String payload;
    info.prettyPrintTo(payload);

    return payload;
}

void Info::publish() {
    this->device->publish(Device::TOPIC_BASE + "/info", this->dump(), true);
}

#ifdef INFO_HTTP_ENABLED
void Info::onHttpIndex(HttpRequest &request, HttpResponse &response) {
    response.setContentType("application/json");
    response.sendString(this->dump());
}
#endif
