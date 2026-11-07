#pragma once

#include "Common.h"
#include <chrono>

class SpellCastHandler : public RE::BSTEventSink<RE::TESSpellCastEvent>
{
public:
    static SpellCastHandler* GetSingleton();

    void Register();
    void Unregister();

    // Event sink
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESSpellCastEvent* a_event,
        RE::BSTEventSource<RE::TESSpellCastEvent>* a_eventSource) override;

    // Settings for throttled notifications (configurable)
    void SetNotificationInterval(float seconds) { m_notificationIntervalSeconds = seconds; }
    float GetNotificationInterval() const { return m_notificationIntervalSeconds; }
    
    void SetWeakenedNotificationsEnabled(bool enabled) { m_weakenedNotificationsEnabled = enabled; }
    bool GetWeakenedNotificationsEnabled() const { return m_weakenedNotificationsEnabled; }

    void SetSpellCastXPNotificationsEnabled(bool enabled) { m_spellCastXPNotificationsEnabled = enabled; }
    bool GetSpellCastXPNotificationsEnabled() const { return m_spellCastXPNotificationsEnabled; }

private:
    SpellCastHandler() = default;
    ~SpellCastHandler() = default;
    SpellCastHandler(const SpellCastHandler&) = delete;
    SpellCastHandler& operator=(const SpellCastHandler&) = delete;

    bool m_registered = false;
    
    // Configurable notification settings
    float m_notificationIntervalSeconds = 10.0f;  // Default: Show weakened spell notification every 10 seconds
    bool m_weakenedNotificationsEnabled = true;   // Default: enabled
    bool m_spellCastXPNotificationsEnabled = true; // Default: Show XP/power notifications when casting learning spells
    
    // Throttled notification tracking
    std::chrono::steady_clock::time_point m_lastWeakenedNotifTime;
    float m_accumulatedXP = 0.0f;  // XP accumulated since last notification
    RE::FormID m_lastNotifiedSpell = 0;  // Track which spell we last notified about
};
