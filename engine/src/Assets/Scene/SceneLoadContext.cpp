#include "Assets/Scene/SceneLoadContext.h"

namespace eng
{
    
void SceneParseContext::ReportError(
    SceneLoadErrorCode code, 
    std::string message, std::string jsonPath,
    std::optional<std::size_t> parseOffset) const {
    errors.push_back({
        .sourcePath = sourcePath,
        .jsonPath = jsonPath,
        .code = code,
        .message = message,
        .parseOffset = parseOffset
    });
}

void SceneDependencyContext::ReportError(
    SceneLoadErrorCode code, 
    std::string message,
    std::string referencedPath) const {
    if (!referencedPath.empty()) {
        message += " ('" + referencedPath + "')";
    }
    errors.push_back(SceneLoadError{
        sourcePath, jsonPath, code, std::move(message), std::nullopt
    });
}

SceneLoadError SceneLoadContext::MakeError(
    SceneLoadErrorCode code, std::string message) const {
    return SceneLoadError{
        sourcePath, jsonPath, code, std::move(message), std::nullopt
    };
}


} // namespace eng
