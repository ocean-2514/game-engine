#ifndef O_FILE_SYSTEM
#define O_FILE_SYSTEM

#include <filesystem>
#include <vector>
#include <string>

namespace eng {
    
class FileSystem
{
public:
    std::filesystem::path GetExecutableFolder() const;
    std::filesystem::path GetAssetsFolder() const;

    std::vector<char> LoadFile(const std::filesystem::path& path) const;
    std::vector<char> LoadAssetFile(const std::string& relativePath) const;
    std::string LoadAssetTextFile(const std::string& relativePath) const;

    bool IsValidAssetPath(const std::string& relativePath) const;
    std::string NormalizeAssetPath(const std::string& relativePath) const;
};




} // namespace eng


#endif // O_FILE_SYSTEM
