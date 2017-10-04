
#include <Wiring/WString.h>
#include "Update.h"
#include "../Device.h"


const char UPDATE_NAME[] = "update";

const char UPDATE_URL_VERSION[] = (( UPDATE_URL "/" DEVICE ".version" ));
const char UPDATE_URL_ROM_0[] = (( UPDATE_URL "/" DEVICE ".rom0" ));
const char UPDATE_URL_ROM_1[] = (( UPDATE_URL "/" DEVICE ".rom1" ));


Update::Update(Device* device) :
        Service(device),
        updater(nullptr) {
    // Receive update messages
    this->device->registerProperty(MQTT_REALM + String("/$broadcast/update"), NodeProperty(Device::MessageCallback(&Update::onUpdateRequestReceived, this), PropertyDataType::Command, ""));
    this->device->registerProperty(Device::TOPIC_BASE + String("/$update"), NodeProperty(Device::MessageCallback(&Update::onUpdateRequestReceived, this), PropertyDataType::Command, ""));

    // Check for updates regularly
    this->timer.initializeMs(UPDATE_INTERVAL, TimerDelegate(&Update::checkUpdate, this));
}

Update::~Update() {
}

void Update::checkUpdate() {
    debugf("Downloading version file");
    this->http.downloadString(UPDATE_URL_VERSION, RequestCompletedDelegate(&Update::onVersionReceived, this));
}

void Update::onUpdateRequestReceived(const String& topic, const String& message) {
    debugf("Request for update received");
    this->checkUpdate();
}

int Update::onVersionReceived(HttpConnection& client, bool successful) {
    if (!successful) {
        debugf("Version download failed");
        return -1;
    }

    debugf("Got version file");

    // Get the first line received
    String version = client.getResponseString();
    version = version.substring(0, version.indexOf('\n') - 1);

    // Compare latest version with current one
    if (version == VERSION) {
        debugf("Already up to date");
        return -1;
    }

    debugf("Remote version differs - updating...");

    // Select rom slot to flash
    const rboot_config bootconf = rboot_get_config();

    // Ensure we have a clean updater
    if (this->updater) delete this->updater;
    this->updater = new rBootHttpUpdate();

    // Configure updater with items to flash
    if (bootconf.current_rom == 0) {
        debugf("Flashing %s to %x", UPDATE_URL_ROM_1, bootconf.roms[1]);
        this->updater->addItem(bootconf.roms[1], UPDATE_URL_ROM_1);
        this->updater->switchToRom(1);
    } else {
        debugf("Flashing %s to %x", UPDATE_URL_ROM_0, bootconf.roms[0]);
        this->updater->addItem(bootconf.roms[0], UPDATE_URL_ROM_0);
        this->updater->switchToRom(0);
    }

    // Start update
    debugf("Downloading update");
    this->updater->start();

    return 0;
}
