#include "HUDProgressSource.h"
#include "Common.h"
#include "uimanager/UIManager.h"
#include "ProgressionManager.h"
#include "ThreadUtils.h"
#include "SKSE/SKSE.h"

using json = nlohmann::json;

namespace SpellLearning {

HUDProgressSource* HUDProgressSource::s_singleton = nullptr;

HUDProgressSource* HUDProgressSource::GetSingleton()
{
    if (!s_singleton) {
        s_singleton = new HUDProgressSource();
    }
    return s_singleton;
}

void HUDProgressSource::Initialize()
{
    BaseXPSource::Initialize();
    m_running = true;
    m_pollThread = std::thread([this]() { PollLoop(); });
    logger::info("HUDProgressSource: Started poll thread");
}

void HUDProgressSource::Shutdown()
{
    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
    BaseXPSource::Shutdown();
}

void HUDProgressSource::SetSettings(const Settings& settings)
{
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_settings = settings;
}

void HUDProgressSource::OnGameLoad()
{
    logger::info("HUDProgressSource: Game load notification received");
    // Push current progress immediately so spells registered on load show up right away
    AddTaskToGameThread("HUDProgress", [this]() {
        PushHUDProgress();
    });
}

void HUDProgressSource::PollLoop()
{
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        Settings local;
        {
            std::lock_guard<std::mutex> lock(m_settingsMutex);
            local = m_settings;
        }

        if (!local.enabled) {
            continue;
        }

        AddTaskToGameThread("HUDProgress", [this]() {
            PushHUDProgress();
        });
    }
}

void HUDProgressSource::PushHUDProgress()
{
    auto* ui = UIManager::GetSingleton();
    if (!ui) {
        return;
    }

    std::string jsonStr = BuildHUDProgressJSON();
    if (jsonStr.empty()) {
        return;
    }

    ui->SendHUDProgressData(jsonStr);
}

std::string HUDProgressSource::BuildHUDProgressJSON()
{
    json result = json::object();
    json schools = json::array();

    const std::vector<std::string> schoolNames = {
        "Destruction", "Restoration", "Alteration", "Conjuration", "Illusion"
    };

    for (const auto& school : schoolNames) {
        RE::FormID targetId = ProgressionManager::GetSingleton()->GetLearningTarget(school);
        if (targetId == 0) {
            continue;
        }

        auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(targetId);
        if (!spell) {
            continue;
        }

        auto progress = ProgressionManager::GetSingleton()->GetProgress(targetId);

        json entry;
        entry["school"] = school;
        entry["formId"] = std::format("0x{:08X}", targetId);
        entry["name"] = spell->GetName();
        entry["progressPercent"] = progress.progressPercent;
        entry["currentXP"] = progress.GetCurrentXP();
        entry["requiredXP"] = progress.requiredXP;
        entry["unlocked"] = progress.unlocked;
        entry["ready"] = !progress.unlocked && progress.progressPercent >= 1.0f;

        schools.push_back(entry);
    }

    result["schools"] = schools;
    result["enabled"] = true;
    return result.dump();
}

} // namespace SpellLearning
