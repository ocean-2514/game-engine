#include "Assets/TextureAssetLoader.h"

#include "Assets/ImageLoader.h"
#include "IO/FileSystem.h"
#include "Renderer/RenderDevice.h"
#include "Renderer/Texture.h"

#include <functional>
#include <iostream>
#include <utility>

namespace eng {

namespace {

template<typename T>
void HashCombine(std::size_t& seed, const T& value) {
	seed ^= std::hash<T>{}(value) + 0x9e3779b9u +
		(seed << 6) + (seed >> 2);
}

} // namespace

TextureAssetLoader::TextureAssetLoader(
	const FileSystem& fileSystem,
	RenderDevice& renderDevice)
	: m_fileSystem(fileSystem),
	  m_renderDevice(renderDevice) {}

bool TextureAssetLoader::TextureCacheKey::operator==(
	const TextureCacheKey& other) const {
	return path == other.path &&
		srgb == other.srgb &&
		flipVertically == other.flipVertically &&
		generateMipmaps == other.generateMipmaps &&
		sampler.addressU == other.sampler.addressU &&
		sampler.addressV == other.sampler.addressV &&
		sampler.minFilter == other.sampler.minFilter &&
		sampler.magFilter == other.sampler.magFilter &&
		sampler.mipFilter == other.sampler.mipFilter;
}

std::size_t TextureAssetLoader::TextureCacheKeyHash::operator()(
	const TextureCacheKey& key) const noexcept {
	std::size_t seed = 0;
	HashCombine(seed, key.path);
	HashCombine(seed, key.srgb);
	HashCombine(seed, key.flipVertically);
	HashCombine(seed, key.generateMipmaps);
	HashCombine(seed, static_cast<int>(key.sampler.addressU));
	HashCombine(seed, static_cast<int>(key.sampler.addressV));
	HashCombine(seed, static_cast<int>(key.sampler.minFilter));
	HashCombine(seed, static_cast<int>(key.sampler.magFilter));
	HashCombine(seed, static_cast<int>(key.sampler.mipFilter));
	return seed;
}

std::shared_ptr<Texture> TextureAssetLoader::LoadTexture(
	const TextureAssetDesc& desc) const {
	const std::string normalizedPath =
		m_fileSystem.NormalizeAssetPath(desc.path);
	if (normalizedPath.empty()) {
		std::cout << "TextureAssetLoader: texture asset path is invalid: "
				  << desc.path << '\n';
		return nullptr;
	}

	TextureCacheKey key{
		normalizedPath,
		desc.loadOptions.srgb,
		desc.loadOptions.flipVertically,
		desc.loadOptions.generateMipmaps,
		desc.samplerDesc};
	const auto cached = m_textureCache.find(key);
	if (cached != m_textureCache.end()) {
		if (auto texture = cached->second.lock()) {
			return texture;
		}
		m_textureCache.erase(cached);
	}

	const auto image = LoadAssetImage(
		m_fileSystem, normalizedPath, desc.loadOptions);
	if (!image) {
		return nullptr;
	}
	auto texture = m_renderDevice.CreateTexture(
		image->textureDesc,
		desc.samplerDesc,
		image->pixels.data(),
		image->pixels.size());
	if (texture) {
		m_textureCache.insert_or_assign(std::move(key), texture);
	}
	return texture;
}

void TextureAssetLoader::ClearCache() {
	m_textureCache.clear();
}

} // namespace eng

namespace eng
{
    





    
} // namespace eng
