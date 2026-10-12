#include "Assets/SceneLoadContext.h"

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

SceneLoadError SceneDependencyContext::ReportError(
    SceneLoadErrorCode code, std::string message) const {
    return SceneLoadError{
        std::string(sourcePath), jsonPath, code,
        std::move(message), std::nullopt
    };
}


} // namespace eng
