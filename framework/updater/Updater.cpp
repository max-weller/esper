#include "Updater.h"


#define UPDATER_URL_ROM(slot) (( UPDATER_URL "/" DEVICE ".rom" slot ))
#define UPDATER_URL_VERSION (( UPDATER_URL "/" DEVICE ".version" ))




static HttpClient http;
static rBootHttpUpdate updater;


void onVersion(HttpClient& client, bool successful) {
    if (!successful) {
        debug_e("Version download failed");
        return;
    }

    debug_i("Got version file");

    // Get the first line received
    String version = client.getResponseString();
    version = version.substring(0, version.indexOf('\n') - 1);

    // Compare latest version with current one
    if (version == VERSION) {
        debug_w("Already up to date");
        return;
    }

    debug_i("Remote version differs - updating...");

    // Select rom slot to flash
    const rboot_config bootconf = rboot_get_config();

    // Add items to flash
    if (bootconf.current_rom == 0) {
        debug_i("Flashing %s to 0x%08X", UPDATER_URL_ROM("1"), bootconf.roms[1]);
        updater.addItem(bootconf.roms[1], UPDATER_URL_ROM("1"));
        updater.switchToRom(1);
    } else {
        debug_i("Flashing %s to 0x%08X", UPDATER_URL_ROM("0"), bootconf.roms[0]);
        updater.addItem(bootconf.roms[0], UPDATER_URL_ROM("0"));
        updater.switchToRom(0);
    }

    // Start update
    debug_w("Downloading update");
    updater.start();
}


void update() {
    debug_i("Starting");

    debug_i("Downloading version file");
    http.downloadString(UPDATER_URL_VERSION, onVersion);
}
