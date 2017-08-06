#include "Info.h"
#include "../Device.h"


const char INFO_NAME[] = "info";

Info::Info(Device* const device)
        : Service(device) {
    // Publish status regularly
    this->timer.initializeMs(60000, TimerDelegate(&Info::publish, this));

    // Record time then starting up
    this->startupTime = RTC.getRtcSeconds();

    debug_i("");
    debug_i("%s", this->getInfo());
    debug_i("");

#ifdef HTTP_PORT
	this->device->http.addPath("/info", HttpPathDelegate(&Info::onHttp_Info, this));
#endif
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

String Info::getInfo() {
    // Read bootloader config
    const rboot_config rbootconf = rboot_get_config();

    return StringSumHelper("") +
            "Device: " + DEVICE + "\n" +
            "ESPer Version: " + VERSION + "\n" +
            "SDK Version: " + system_get_sdk_version() + "\n" +
            "Boot: v" + String(system_get_boot_version()) + " (" + String(system_get_boot_mode()) + ")\n" +
            "System Chip ID: " + String(system_get_chip_id(), 16) + "\n" +
            "IP: " + WifiStation.getIP().toString() + "\n" +
            "MAC: " + WifiStation.getMAC() + "\n" +
            "SPI Flash ID: " + String(spi_flash_get_id(), 16) + "\n" +
            "CPU Frequency: " + String(system_get_cpu_freq()) + " MHz\n" +
            "ROM Selected: " + String(rboot_get_current_rom()) + "\n" +
            "ROM Slots: " + String(rbootconf.roms[0], 16) + "/" + String(rbootconf.roms[1], 16) + "\n" +
            "Startup Time: " + String(this->startupTime) + "\n" +
            "Connect Time: " + String(this->connectTime) + "\n" +
            "Updated Time: " + String(RTC.getRtcSeconds()) + "\n";
}

void Info::publish() {
    LOG.log("Publishing device info");

    this->device->publish(Device::TOPIC_BASE + "/info", this->getInfo(), true);
}

#ifdef HTTP_PORT
void Info::onHttp_Info(HttpRequest &request, HttpResponse &response)
{
    response.setContentType(ContentType::TEXT);
    response.setHeader("Server", "ham");
	
    response.sendString(this->getInfo());

    char s[128];
    for (int i = 0; i < this->device->services.count(); i++) {
        sprintf(s, "Service: %s\n", this->device->services[i]->getName() );
        response.sendString(s);
    }

    for (unsigned int i = 0; i < this->device->messageCallbacks.count(); i++) {
        sprintf(s, "Endpoint: %s\n", this->device->messageCallbacks.keyAt(i).c_str() );
        response.sendString(s);
    }
}
#endif
