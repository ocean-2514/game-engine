#ifndef O_SCENE_LOAD_RESULT
#define O_SCENE_LOAD_RESULT

#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace eng
{

enum class SceneLoadErrorCode {
    Io, JsonSyntax, Schema, UnsupportedVersion, 
    UnknownType, InvalidReference, AssetNotFound, 
    Construction
};
    
struct SceneLoadError {
    std::string sourcePath;
    std::string jsonPath;   // $.objects[2].components[0].material
    SceneLoadErrorCode code = SceneLoadErrorCode::Schema;
    std::string message;
    std::optional<std::size_t> parseOffset;
};

template<typename T>
struct SceneLoadResult {
    static_assert(!std::is_reference_v<T>,
        "SceneLoadResult cannot store a reference type");
    static_assert(!std::is_same_v<std::remove_cv_t<T>,
            std::vector<SceneLoadError>>,
        "SceneLoadResult value type cannot be its error-list type");

    static SceneLoadResult Success(T value) {
        return SceneLoadResult(std::move(value));
    }

    static SceneLoadResult Failure(SceneLoadError error) {
        std::vector<SceneLoadError> errors;
        errors.push_back(std::move(error));
        return Failure(std::move(errors));
    }

    static SceneLoadResult Failure(std::vector<SceneLoadError> errors) {
        EnsureFailureHasDiagnostic(errors);
        return SceneLoadResult(std::move(errors));
    }

    [[nodiscard]] bool HasValue() const noexcept {
        return std::holds_alternative<T>(data);
    }

    [[nodiscard]] bool HasError() const noexcept {
        return std::holds_alternative<std::vector<SceneLoadError>>(data);
    }

    explicit operator bool() const noexcept { return HasValue(); }

    T* GetValue() {
        return std::get_if<T>(&data);
    }
    const T* GetValue() const {
        return std::get_if<T>(&data);
    }

    T& Value() { return std::get<T>(data); }
    const T& Value() const { return std::get<T>(data); }

    const std::vector<SceneLoadError>* GetErrors() const {
        return std::get_if<std::vector<SceneLoadError>>(&data);
    }

private:
    static void EnsureFailureHasDiagnostic(
        std::vector<SceneLoadError>& errors) {
        if (errors.empty()) {
            errors.push_back(SceneLoadError{
                {}, {}, SceneLoadErrorCode::Construction,
                "SceneLoadResult::Failure requires at least one diagnostic",
                std::nullopt
            });
        }
    }

    explicit SceneLoadResult(T value)
        : data(std::in_place_index<0>, std::move(value)) {}

    explicit SceneLoadResult(std::vector<SceneLoadError> errors)
        : data(std::in_place_index<1>, std::move(errors)) {}

    std::variant<T, std::vector<SceneLoadError>> data;
};

template<>
struct SceneLoadResult<void> {
    static SceneLoadResult Success() {
        return SceneLoadResult(std::monostate{});
    }

    static SceneLoadResult Failure(SceneLoadError error) {
        std::vector<SceneLoadError> errors;
        errors.push_back(std::move(error));
        return Failure(std::move(errors));
    }

    static SceneLoadResult Failure(std::vector<SceneLoadError> errors) {
        EnsureFailureHasDiagnostic(errors);
        return SceneLoadResult(std::move(errors));
    }

    [[nodiscard]] bool HasValue() const noexcept {
        return std::holds_alternative<std::monostate>(data);
    }

    [[nodiscard]] bool HasError() const noexcept {
        return std::holds_alternative<std::vector<SceneLoadError>>(data);
    }

    explicit operator bool() const noexcept { return HasValue(); }

    const std::vector<SceneLoadError>* GetErrors() const {
        return std::get_if<std::vector<SceneLoadError>>(&data);
    }

private:
    static void EnsureFailureHasDiagnostic(
        std::vector<SceneLoadError>& errors) {
        if (errors.empty()) {
            errors.push_back(SceneLoadError{
                {}, {}, SceneLoadErrorCode::Construction,
                "SceneLoadResult::Failure requires at least one diagnostic",
                std::nullopt
            });
        }
    }

    explicit SceneLoadResult(std::monostate value)
        : data(std::in_place_index<0>, value) {}

    explicit SceneLoadResult(std::vector<SceneLoadError> errors)
        : data(std::in_place_index<1>, std::move(errors)) {}

    std::variant<std::monostate, std::vector<SceneLoadError>> data;
};

} // namespace eng


#endif // O_SCENE_LOAD_RESULT
