#include "IO/FileSystem.h"
#include "config.h"
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#elif defined (__APPLE__)
#include <mach-o/dyld.h>
#elif defined (__linux__) 
#include <unistd.h>
#include <limits.h>
#endif

namespace eng {
    
std::filesystem::path FileSystem::GetExecutableFolder() const {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).remove_filename();
#elif defined (__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string tem(size, '\0');
    _NSGetExecutablePath(tem.data(), &size);
    return std::filesystem::weakly_canonical(std::filesystem::path(tem)).remove_filename();
#elif defined (__linux__) 
    return std::filesystem::weakly_canonical(std::filesystem::read_symlink("/proc/self/exe")).remove_filename();
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path FileSystem::GetAssetsFolder() const {
#ifdef ASSETS_ROOT
    auto path = std::filesystem::path(std::string(ASSETS_ROOT));
    if (std::filesystem::exists(path)) {
        return path;
    }
#endif
    return std::filesystem::weakly_canonical(GetExecutableFolder() / "assets");
}

std::vector<char> FileSystem::LoadFile(const std::filesystem::path& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<char> buffer(size);

    if (!file.read(buffer.data(), size)) {
        return {};
    }

    return buffer;
}

std::vector<char> FileSystem::LoadAssetFile(const std::string& relativePath) const {
    if (!IsValidAssetPath(relativePath)) {
        return {};
    }
    return LoadFile(GetAssetsFolder() / relativePath);
}

std::string FileSystem::LoadAssetTextFile(const std::string& relativePath) const {
    auto buffer = LoadAssetFile(relativePath);
    return std::string(buffer.begin(), buffer.end());
}

bool FileSystem::IsValidAssetPath(const std::string& relativePath) const {
    return !NormalizeAssetPath(relativePath).empty();
}

std::string FileSystem::NormalizeAssetPath(
    const std::string& relativePath) const {
    const std::filesystem::path input(relativePath);
    if (input.empty() || input.is_absolute()) {
        return {};
    }

    std::error_code error;
    const auto root = std::filesystem::weakly_canonical(
        GetAssetsFolder(), error);
    if (error) return {};
    const auto path = std::filesystem::weakly_canonical(
        root / input, error);
    if (error) return {};

    const auto relative = path.lexically_relative(root);
    if (relative.empty() || *relative.begin() == "..") {
        return {};
    }
    if (!std::filesystem::is_regular_file(path, error) || error) {
        return {};
    }
    return relative.generic_string();
}



} // namespace eng
