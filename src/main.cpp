#include <cstdlib>
#include <string>
#include <thread>
#if defined(_WIN32)
#include <winsock2.h>
#include <websocketpp/error.hpp>
#endif

#include "lib/json/json.hpp"
#include "lib/easylogging/easylogging++.h"
#include "lib/filedialogs/portable-file-dialogs.h"
#include "auth/permission.h"
#include "auth/authbasic.h"
#include "server/neuserver.h"
#include "settings.h"
#include "resources.h"
#include "helpers.h"
#include "chrome.h"
#include "extensions_loader.h"
#include "api/app/app.h"
#include "api/window/window.h"
#include "api/os/os.h"
#include "api/debug/debug.h"

#define NEU_APP_LOG_FILE "/neutralinojs.log"
#define NEU_APP_LOG_FORMAT "%level %datetime %msg %loc %user@%host"
#define ELPP_THREAD_SAFE

INITIALIZE_EASYLOGGINGPP

using namespace std;
using json = nlohmann::json;

string navigationUrl = "";

void __wait() {
    while (true)
    {
        this_thread::sleep_for(20000ms);
    }
}

void __startApp(const json& args) {
    json options = settings::getConfig();

    if (options.contains("startEXE"))
    {
        string startEXE = options["startEXE"];
        std::vector<string> args2 = args;
        string args_str = "";
        for (int i = 1; i < args2.size(); i++)
        {
            args_str += args2[i] + " ";
        }
        string exe_filename = filesystem::path(startEXE).filename().string();
        if (filesystem::exists(exe_filename)) startEXE = exe_filename;
        os::execCommand(startEXE, args_str, true);
    }

    switch (settings::getMode())
    {
    case settings::AppModeBrowser:
        os::open(navigationUrl);
        __wait();
        break;
    case settings::AppModeWindow:
    {
        json windowOptions = options["modes"]["window"];
        windowOptions["url"] = navigationUrl;
        window::controllers::init(windowOptions);
    }
    break;
    case settings::AppModeCloud:
        if (neuserver::isInitialized())
        {
            debug::log(debug::LogTypeInfo, settings::getAppId() +
                " is available at " + navigationUrl);
        }
        __wait();
        break;
    case settings::AppModeChrome:
    {
        json chromeOptions = options["modes"]["chrome"];
        chromeOptions["url"] = navigationUrl;
        chrome::init(chromeOptions);
        __wait();
    }
    break;
    }
}

void __configureLogger() {
    bool enableLogging = true;
    bool enableLogFile = true;

    json logging = settings::getOptionForCurrentMode("logging");
    if (!logging["enabled"].is_null())
    {
        enableLogging = logging["enabled"].get<bool>();
    }
    if (!logging["writeToLogFile"].is_null())
    {
        enableLogFile = logging["writeToLogFile"].get<bool>();
    }

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally(
        el::ConfigurationType::Format, NEU_APP_LOG_FORMAT);

    if (enableLogFile)
    {
        defaultConf.setGlobally(
            el::ConfigurationType::Filename, settings::joinAppPath(NEU_APP_LOG_FILE));
    }
    defaultConf.setGlobally(
        el::ConfigurationType::ToFile, enableLogFile ? "true" : "false");

    defaultConf.setGlobally(
        el::ConfigurationType::Enabled, enableLogging ? "true" : "false");
    el::Loggers::reconfigureLogger("default", defaultConf);
}

void __startServerAsync() {
    navigationUrl = settings::getNavigationUrl();
    json jEnableServer = settings::getOptionForCurrentMode("enableServer");

    if (!jEnableServer.is_null() && jEnableServer.get<bool>())
    {
        try
        {
            navigationUrl = neuserver::init();
        }
        catch (websocketpp::exception& e)
        {
            json jPort = settings::getOptionForCurrentMode("port");
            string errorMsg = "Neutralinojs can't initialize the application server";
            if (!jPort.is_null())
            {
                errorMsg += " on port: " + to_string(jPort.get<int>());
            }
            pfd::message("Unable to start server",
                errorMsg,
                pfd::choice::ok,
                pfd::icon::error);
            std::exit(1);
        }
        neuserver::startAsync();
    }
}

void __initFramework(const json& args) {
    settings::setGlobalArgs(args);
    resources::init();
    bool settingsStatus = settings::init();
    if (!settingsStatus)
    {
        pfd::message("Unable to load configuration",
            "The application configuration file cannot be loaded due to a JSON parsing error.",
            pfd::choice::ok,
            pfd::icon::error);
        std::exit(1);
    }
    authbasic::init();
    permission::init();
}

void __initExtra() {
    bool enableExtensions = false;
    bool exportAuthInfo = false;
    json exts = settings::getOptionForCurrentMode("enableExtensions");
    if (!exts.is_null())
    {
        enableExtensions = exts.get<bool>();
    }
    json exportAuth = settings::getOptionForCurrentMode("exportAuthInfo");
    if (!exportAuth.is_null())
    {
        exportAuthInfo = exportAuth.get<bool>();
    }

    if (exportAuthInfo)
    {
        authbasic::exportAuthInfo();
    }
    if (enableExtensions)
    {
        extensions::init();
    }
}

#if defined(_WIN32)
void __attachConsole() {
    FILE* fp;
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen_s(&fp, "CONIN$", "r", stdin);
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }
}

#define ARG_C __argc
#define ARG_V __wargv
#define CONVWCSTR(S) helpers::wcstr2str(S)
int APIENTRY wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#define ARG_C argc
#define ARG_V argv
#define CONVWCSTR(S) S
int main(int argc, char** argv)
#endif
{
    json args;
    for (int i = 0; i < ARG_C; i++)
    {
        args.push_back(CONVWCSTR(ARG_V[i]));
    }
#if defined(_WIN32)
    __attachConsole();
#endif
    __initFramework(args);
    __startServerAsync();
    __configureLogger();
    __initExtra();
    __startApp(args);
    return 0;
}
