#include "Common.h"
#include "uimanager/UIManager.h"
#include "EncodingUtils.h"
#include "ThreadUtils.h"

#include <unordered_map>
#include <set>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// =============================================================================
// CLIPBOARD CALLBACKS (Windows API)
// =============================================================================

void UIManager::OnCopyToClipboard(const char* argument)
{
    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: CopyToClipboard - no content provided");
        return;
    }

    logger::info("UIManager: CopyToClipboard ({} bytes)", strlen(argument));

    std::string argStr(argument);

    AddTaskToGameThread("CopyToClipboard", [argStr]() {
        auto* instance = GetSingleton();
        if (!instance || !instance->m_prismaUI) return;

        bool success = false;

        // Convert UTF-8 to UTF-16 for Windows clipboard
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, argStr.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) {
            logger::error("UIManager: MultiByteToWideChar failed to compute length");
            instance->NotifyCopyComplete(false);
            return;
        }

        if (OpenClipboard(nullptr)) {
            EmptyClipboard();

            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<size_t>(wideLen) * sizeof(WCHAR));
            if (hMem) {
                WCHAR* pMem = static_cast<WCHAR*>(GlobalLock(hMem));
                if (pMem) {
                    MultiByteToWideChar(CP_UTF8, 0, argStr.c_str(), -1, pMem, wideLen);
                    GlobalUnlock(hMem);

                    if (SetClipboardData(CF_UNICODETEXT, hMem)) {
                        success = true;
                        logger::info("UIManager: Successfully copied to clipboard");
                    } else {
                        logger::error("UIManager: SetClipboardData failed");
                        GlobalFree(hMem);
                    }
                } else {
                    logger::error("UIManager: GlobalLock failed");
                    GlobalFree(hMem);
                }
            } else {
                logger::error("UIManager: GlobalAlloc failed");
            }

            CloseClipboard();
        } else {
            logger::error("UIManager: OpenClipboard failed");
        }

        instance->NotifyCopyComplete(success);
    });
}

void UIManager::OnGetClipboard([[maybe_unused]] const char* argument)
{
    logger::info("UIManager: GetClipboard callback triggered");

    AddTaskToGameThread("GetClipboard", []() {
        auto* instance = GetSingleton();
        if (!instance || !instance->m_prismaUI) return;

        std::string content;

        // Use Windows clipboard API with UTF-16
        if (OpenClipboard(nullptr)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);

            if (hData) {
                WCHAR* pWideText = static_cast<WCHAR*>(GlobalLock(hData));
                if (pWideText) {
                    // Convert UTF-16 back to UTF-8
                    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, pWideText, -1, nullptr, 0, nullptr, nullptr);
                    if (utf8Len > 1) {
                        content.resize(static_cast<size_t>(utf8Len));
                        WideCharToMultiByte(CP_UTF8, 0, pWideText, -1, content.data(), utf8Len, nullptr, nullptr);
                        content.pop_back();  // Remove null terminator written by WideCharToMultiByte
                    }
                    GlobalUnlock(hData);
                    logger::info("UIManager: Read {} bytes from clipboard", content.size());
                } else {
                    logger::warn("UIManager: GlobalLock failed on clipboard data");
                }
            } else {
                logger::warn("UIManager: No text data in clipboard");
            }

            CloseClipboard();
        } else {
            logger::error("UIManager: OpenClipboard failed");
        }

        // Send content to UI (even if empty)
        instance->SendClipboardContent(content);
    });
}

// =============================================================================
// PRESET FILE I/O
// =============================================================================

static std::filesystem::path GetPresetsBasePath()
{
    return "Data/SKSE/Plugins/SpellLearning/presets";
}

void UIManager::OnSavePreset(const char* argument)
{
    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: SavePreset - no data provided");
        return;
    }

    std::string argStr(argument);

    AddTaskToGameThread("SavePreset", [argStr]() {
        try {
            json args = json::parse(argStr);
            std::string type = args.value("type", "");
            std::string name = args.value("name", "");
            json data = args.value("data", json::object());

            if (type.empty() || name.empty()) {
                logger::warn("UIManager: SavePreset - missing type or name");
                return;
            }

            std::string safeName = EncodingUtils::SanitizeFilename(name);
            // Sanitize type to prevent path traversal
            std::string safeType = EncodingUtils::SanitizeFilename(type);

            // Defense-in-depth: reject if sanitized values still contain ".."
            if (safeName.find("..") != std::string::npos || safeType.find("..") != std::string::npos) {
                logger::error("UIManager: SavePreset - rejected suspicious name/type");
                return;
            }

            auto dir = GetPresetsBasePath() / safeType;
            std::filesystem::create_directories(dir);

            auto filePath = dir / (safeName + ".json");
            std::ofstream file(filePath);
            if (!file.is_open()) {
                logger::error("UIManager: SavePreset - failed to open {}", filePath.string());
                return;
            }
            file << data.dump(2);
            file.close();

            logger::info("UIManager: SavePreset - saved {}/{}.json", type, safeName);
        } catch (const std::exception& e) {
            logger::error("UIManager: SavePreset exception: {}", e.what());
        }
    });
}

void UIManager::OnDeletePreset(const char* argument)
{
    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: DeletePreset - no data provided");
        return;
    }

    std::string argStr(argument);

    AddTaskToGameThread("DeletePreset", [argStr]() {
        try {
            json args = json::parse(argStr);
            std::string type = args.value("type", "");
            std::string name = args.value("name", "");

            if (type.empty() || name.empty()) {
                logger::warn("UIManager: DeletePreset - missing type or name");
                return;
            }

            std::string safeName = EncodingUtils::SanitizeFilename(name);
            // Sanitize type to prevent path traversal
            std::string safeType = EncodingUtils::SanitizeFilename(type);

            // Defense-in-depth: reject if sanitized values still contain ".."
            if (safeName.find("..") != std::string::npos || safeType.find("..") != std::string::npos) {
                logger::error("UIManager: DeletePreset - rejected suspicious name/type");
                return;
            }

            auto filePath = GetPresetsBasePath() / safeType / (safeName + ".json");

            if (std::filesystem::exists(filePath)) {
                std::filesystem::remove(filePath);
                logger::info("UIManager: DeletePreset - deleted {}/{}.json", type, safeName);
            } else {
                logger::warn("UIManager: DeletePreset - file not found: {}", filePath.string());
            }
        } catch (const std::exception& e) {
            logger::error("UIManager: DeletePreset exception: {}", e.what());
        }
    });
}

void UIManager::OnLoadPresets(const char* argument)
{
    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: LoadPresets - no data provided");
        return;
    }

    // Copy argument — must defer via AddTask to avoid re-entrant JS calls.
    // InteropCall back into JS from within a RegisterJSListener callback
    // doesn't work in Ultralight (re-entrant), so we defer to SKSE task thread.
    std::string argStr(argument);

    AddTaskToGameThread("LoadPresets", [argStr]() {
        auto* instance = GetSingleton();
        if (!instance || !instance->m_prismaUI) return;

        try {
            json args = json::parse(argStr);
            std::string type = args.value("type", "");

            if (type.empty()) {
                logger::warn("UIManager: LoadPresets - missing type");
                return;
            }

            // Sanitize type to prevent path traversal
            std::string safeType = EncodingUtils::SanitizeFilename(type);

            // Defense-in-depth: reject if sanitized value still contains ".."
            if (safeType.find("..") != std::string::npos) {
                logger::error("UIManager: LoadPresets - rejected suspicious type");
                return;
            }

            auto dir = GetPresetsBasePath() / safeType;
            json result;
            result["type"] = type;
            result["presets"] = json::array();

            if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (!entry.is_regular_file()) continue;
                    auto ext = entry.path().extension().string();
                    // Case-insensitive .json check
                    if (ext != ".json" && ext != ".JSON") continue;

                    try {
                        std::ifstream file(entry.path());
                        json presetData = json::parse(file);

                        // Use the filename (without extension) as key, but prefer "name" inside the JSON
                        std::string key = entry.path().stem().string();
                        if (presetData.contains("name") && presetData["name"].is_string()) {
                            key = presetData["name"].get<std::string>();
                        }

                        json presetEntry;
                        presetEntry["key"] = key;
                        presetEntry["data"] = presetData;
                        result["presets"].push_back(presetEntry);

                        logger::info("UIManager: LoadPresets - loaded {}/{}", type, key);
                    } catch (const std::exception& e) {
                        logger::warn("UIManager: LoadPresets - failed to parse {}: {}",
                                     entry.path().string(), e.what());
                    }
                }
            } else {
                // Directory doesn't exist yet - that's fine, return empty array
                logger::info("UIManager: LoadPresets - no presets directory for type '{}'", type);
            }

            std::string resultStr = result.dump();
            logger::info("UIManager: LoadPresets - sending {} {} presets to UI",
                         result["presets"].size(), type);
            instance->m_prismaUI->InteropCall(instance->m_view, "onPresetsLoaded", resultStr.c_str());

        } catch (const std::exception& e) {
            logger::error("UIManager: LoadPresets exception: {}", e.what());
        }
    });
}

// =============================================================================
// AUTO-TEST CALLBACKS
// =============================================================================

void UIManager::OnLoadTestConfig([[maybe_unused]] const char* argument)
{
    logger::info("UIManager: LoadTestConfig callback triggered");

    AddTaskToGameThread("LoadTestConfig", []() {
        auto* instance = GetSingleton();
        if (!instance || !instance->m_prismaUI) return;

        std::filesystem::path configPath = "Data/SKSE/Plugins/SpellLearning/test_config.json";

        try {
            if (!std::filesystem::exists(configPath)) {
                logger::info("UIManager: No test_config.json found - test mode disabled");
                // Send empty/disabled response
                nlohmann::json response;
                response["enabled"] = false;
                instance->m_prismaUI->InteropCall(instance->m_view, "onTestConfigLoaded", response.dump().c_str());
                return;
            }

            std::ifstream file(configPath);
            if (!file.is_open()) {
                logger::error("UIManager: Failed to open test_config.json");
                return;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();

            // Parse and validate
            nlohmann::json config = nlohmann::json::parse(buffer.str());

            logger::info("UIManager: Test config loaded - enabled: {}, preset: {}",
                         config.value("enabled", false),
                         config.value("preset", "unknown"));

            // Send to JS
            instance->m_prismaUI->InteropCall(instance->m_view, "onTestConfigLoaded", config.dump().c_str());

        } catch (const std::exception& e) {
            logger::error("UIManager: Exception loading test config: {}", e.what());
        }
    });
}

void UIManager::OnSaveTestResults(const char* argument)
{
    logger::info("UIManager: SaveTestResults callback triggered");

    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: SaveTestResults - no content");
        return;
    }

    try {
        // Parse the argument to get the results JSON string
        nlohmann::json request = nlohmann::json::parse(argument);
        std::string resultsJson = request.value("results", "{}");

        // Create output directory
        std::filesystem::path outputDir = "Data/SKSE/Plugins/SpellLearning";
        std::filesystem::create_directories(outputDir);

        // Write results file
        std::filesystem::path resultsPath = outputDir / "test_results.json";
        std::ofstream file(resultsPath);
        if (file.is_open()) {
            file << resultsJson;
            file.close();
            logger::info("UIManager: Saved test results to {}", resultsPath.string());
        } else {
            logger::error("UIManager: Failed to open test_results.json for writing");
        }

    } catch (const std::exception& e) {
        logger::error("UIManager: Exception saving test results: {}", e.what());
    }
}

// =============================================================================
// TREE FORMID UPDATER (ports update_tree.py logic to C++, no Python required)
// =============================================================================
//
// Re-maps FormIDs in a spell tree JSON when plugins have been renumbered
// (e.g. after a mod update). Builds an old→new FormID lookup from an old scan
// and a new scan, matching by spell name → plugin+name → persistentId, then
// recursively rewrites every matching FormID string inside the tree.
//
// Called from JS via callCpp("UpdateTreeFormIds", JSON).
// Argument JSON:
//   { "oldScan": { "spells": [...] },
//     "newScan": { "spells": [...] },
//     "tree":    { <full tree JSON> } }
// Response (sent back to JS on onTreeFormIdsUpdated):
//   { "success": bool,
//     "newTree": { <tree with remapped FormIDs> },
//     "stats":   { "mappingsFound": N, "missingMatches": M, "replacementsMade": R },
//     "mismatches": [ { plugin, name, old_formId, new_formId }, ... ],
//     "missing": [ "PLUGIN | name", ... ] }
// =============================================================================

namespace {

    // Recursively replace any string that is a key in formidMap, returning the
    // (possibly modified) json and counting replacements made.
    json ReplaceFormIdsInTree(const json& obj,
                              const std::unordered_map<std::string, std::string>& formidMap,
                              std::set<std::string>& seenIds,
                              const std::unordered_map<std::string, json>& mismatchInfo,
                              std::vector<json>& mismatchRows,
                              size_t& replacementsMade)
    {
        if (obj.is_object()) {
            json result = json::object();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                result[it.key()] = ReplaceFormIdsInTree(it.value(), formidMap, seenIds, mismatchInfo, mismatchRows, replacementsMade);
            }
            return result;
        }
        if (obj.is_array()) {
            json result = json::array();
            for (const auto& elem : obj) {
                result.push_back(ReplaceFormIdsInTree(elem, formidMap, seenIds, mismatchInfo, mismatchRows, replacementsMade));
            }
            return result;
        }
        if (obj.is_string()) {
            std::string s = obj.get<std::string>();
            auto mapIt = formidMap.find(s);
            if (mapIt != formidMap.end()) {
                if (seenIds.insert(s).second) {
                    auto infoIt = mismatchInfo.find(s);
                    if (infoIt != mismatchInfo.end()) {
                        mismatchRows.push_back(infoIt->second);
                    }
                }
                ++replacementsMade;
                return mapIt->second;
            }
        }
        return obj;
    }
}

void UIManager::OnUpdateTreeFormIds(const char* argument)
{
    if (!argument || strlen(argument) == 0) {
        logger::warn("UIManager: UpdateTreeFormIds - no data provided");
        return;
    }

    std::string argStr(argument);

    AddTaskToGameThread("UpdateTreeFormIds", [argStr]() {
        auto* instance = GetSingleton();
        if (!instance || !instance->m_prismaUI) return;

        try {
            json request = json::parse(argStr);

            if (!request.contains("oldScan") || !request["oldScan"].is_object() ||
                !request.contains("newScan") || !request["newScan"].is_object() ||
                !request.contains("tree") || !request["tree"].is_object()) {
                logger::error("UIManager: UpdateTreeFormIds - missing oldScan/newScan/tree");
                nlohmann::json response;
                response["success"] = false;
                response["error"] = "Request must include oldScan, newScan, and tree objects";
                instance->m_prismaUI->InteropCall(instance->m_view, "onTreeFormIdsUpdated", response.dump().c_str());
                return;
            }

            const json& oldScan = request["oldScan"];
            const json& newScan = request["newScan"];
            const json& tree = request["tree"];

            // ------------------------------------------------------------------
            // Build lookup from new scan (name → [formIds], plugin+name → [formIds], persistentId → formId)
            // ------------------------------------------------------------------
            std::unordered_map<std::string, std::vector<std::string>> nameLookup;
            std::unordered_map<std::string, std::vector<std::string>> pluginNameLookup;
            std::unordered_map<std::string, std::string> persistentLookup;

            if (newScan.contains("spells") && newScan["spells"].is_array()) {
                for (const auto& spell : newScan["spells"]) {
                    if (!spell.is_object()) continue;
                    std::string name = spell.value("name", "");
                    std::string plugin = spell.value("plugin", "");
                    std::string formid = spell.value("formId", "");
                    std::string persistentId = spell.value("persistentId", "");

                    if (!formid.empty()) {
                        nameLookup[name].push_back(formid);
                        std::string pnKey = plugin + "|" + name;
                        pluginNameLookup[pnKey].push_back(formid);
                    }
                    if (!persistentId.empty()) {
                        persistentLookup[persistentId] = formid;
                    }
                }
            }

            // Warn on duplicates (mirrors Python behavior, informational only)
            for (auto& [name, ids] : nameLookup) {
                if (ids.size() > 1) {
                    logger::warn("UIManager: Duplicate spell name in new scan: {}", name);
                }
            }
            for (auto& [key, ids] : pluginNameLookup) {
                if (ids.size() > 1) {
                    logger::warn("UIManager: Duplicate spell in new scan for plugin+name: {}", key);
                }
            }

            // ------------------------------------------------------------------
            // Build old → new FormID mapping
            // ------------------------------------------------------------------
            std::unordered_map<std::string, std::string> formidMap;
            std::unordered_map<std::string, json> mismatchInfo;
            std::vector<std::string> missing;

            if (oldScan.contains("spells") && oldScan["spells"].is_array()) {
                for (const auto& spell : oldScan["spells"]) {
                    if (!spell.is_object()) continue;
                    std::string name = spell.value("name", "");
                    std::string plugin = spell.value("plugin", "");
                    std::string oldFormid = spell.value("formId", "");

                    if (oldFormid.empty()) continue;

                    std::string newFormid;

                    auto idsByName = nameLookup.find(name);
                    if (idsByName != nameLookup.end() && idsByName->second.size() == 1) {
                        newFormid = idsByName->second[0];
                    } else {
                        std::string pnKey = plugin + "|" + name;
                        auto idsByPluginName = pluginNameLookup.find(pnKey);
                        if (idsByPluginName != pluginNameLookup.end() && idsByPluginName->second.size() == 1) {
                            newFormid = idsByPluginName->second[0];
                        } else {
                            std::string persistentId = spell.value("persistentId", "");
                            if (!persistentId.empty()) {
                                auto pit = persistentLookup.find(persistentId);
                                if (pit != persistentLookup.end()) {
                                    newFormid = pit->second;
                                    logger::info("UIManager: Using persistentId fallback for {} | {}: {}",
                                        plugin, name, persistentId);
                                }
                            }
                        }
                    }

                    if (newFormid.empty()) {
                        missing.push_back(plugin + " | " + name);
                        continue;
                    }

                    if (oldFormid != newFormid) {
                        formidMap[oldFormid] = newFormid;
                        json info;
                        info["plugin"] = plugin;
                        info["name"] = name;
                        info["old_formId"] = oldFormid;
                        info["new_formId"] = newFormid;
                        mismatchInfo[oldFormid] = info;
                    }
                }
            }

            logger::info("UIManager: UpdateTreeFormIds - mappings found: {}, missing matches: {}",
                formidMap.size(), missing.size());

            // ------------------------------------------------------------------
            // Recursive replacement in tree
            // ------------------------------------------------------------------
            std::set<std::string> seenIds;
            std::vector<json> mismatchRows;
            size_t replacementsMade = 0;

            json newTree = ReplaceFormIdsInTree(tree, formidMap, seenIds, mismatchInfo, mismatchRows, replacementsMade);

            logger::info("UIManager: UpdateTreeFormIds - replacements made: {}", replacementsMade);

            // ------------------------------------------------------------------
            // Send response back to JS
            // ------------------------------------------------------------------
            nlohmann::json response;
            response["success"] = true;
            response["newTree"] = newTree;
            response["stats"]["mappingsFound"] = (int)formidMap.size();
            response["stats"]["missingMatches"] = (int)missing.size();
            response["stats"]["replacementsMade"] = (int)replacementsMade;
            response["mismatches"] = mismatchRows;
            response["missing"] = missing;

            instance->m_prismaUI->InteropCall(instance->m_view, "onTreeFormIdsUpdated", response.dump().c_str());

        } catch (const std::exception& e) {
            logger::error("UIManager: UpdateTreeFormIds exception: {}", e.what());
            nlohmann::json response;
            response["success"] = false;
            response["error"] = e.what();
            if (instance->m_prismaUI) {
                instance->m_prismaUI->InteropCall(instance->m_view, "onTreeFormIdsUpdated", response.dump().c_str());
            }
        } catch (...) {
            logger::error("UIManager: UpdateTreeFormIds unknown exception");
            nlohmann::json response;
            response["success"] = false;
            response["error"] = "Unknown internal error";
            if (instance->m_prismaUI) {
                instance->m_prismaUI->InteropCall(instance->m_view, "onTreeFormIdsUpdated", response.dump().c_str());
            }
        }
    });
}
