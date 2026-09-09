#pragma once

#include <string>
#include <functional>
#include <mutex>

namespace OpenRouterAPI {

    struct Config {
        std::string apiKey;
        std::string model = "anthropic/claude-sonnet-4";
        int maxTokens = 64000;
        bool verifySSL = true;
    };

    struct Response {
        bool success = false;
        std::string content;
        std::string error;
    };

    bool Initialize();
    void Shutdown();
    Config GetConfig();
    void SetConfig(const Config& config);
    std::mutex& GetConfigMutex();
    void SaveConfig();

    void SendPromptAsync(
        const std::string& systemPrompt,
        const std::string& userPrompt,
        std::function<void(const Response&)> callback
    );

    Response SendPrompt(
        const std::string& systemPrompt,
        const std::string& userPrompt
    );

}
