#pragma once

#include "XPSource.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

namespace SpellLearning {

/**
 * HUDProgressSource - Persistent in-game HUD progress bars for spell learning XP
 *
 * Polls on a background thread and pushes per-school learning target progress
 * to the PrismaUI JS overlay via InteropCall ("onHUDProgressUpdate").
 * All game thread access (RE::Calendar, RE::TESForm::LookupByID) is marshaled
 * via AddTaskToGameThread().
 *
 * Settings (from unified config "hudProgress"):
 *  - enabled: master toggle
 *  - position: { x, y } screen position in pixels
 *  - scale: percentage (50-200)
 *  - opacity: percentage (0-100)
 *  - showPerSchool: show one bar per school
 */
class HUDProgressSource : public BaseXPSource {
public:
    struct Settings {
        bool enabled = false;
        int posX = 50;
        int posY = 50;
        int scale = 100;
        int opacity = 100;
        bool showPerSchool = true;
    };

    HUDProgressSource()
        : BaseXPSource(
            "hudProgress",
            "HUD Progress",
            "Shows persistent in-game XP progress bars for learning targets."
          )
    {}

    ~HUDProgressSource() override { Shutdown(); }

    void Initialize() override;
    void Shutdown() override;
    void OnGameLoad();

    // Public: push immediate HUD progress update to JS (called when learning target changes)
    void PushHUDProgress();

    int GetPriority() const override { return 55; }

    void SetSettings(const Settings& settings);
    Settings GetSettings() const {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        return m_settings;
    }

    static HUDProgressSource* GetSingleton();

private:
    void PollLoop();
    std::string BuildHUDProgressJSON();

    Settings m_settings;
    mutable std::mutex m_settingsMutex;

    std::atomic<bool> m_running{false};
    std::thread m_pollThread;

    static HUDProgressSource* s_singleton;
};

} // namespace SpellLearning
