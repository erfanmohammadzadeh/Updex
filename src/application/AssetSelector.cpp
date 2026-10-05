#include "application/AssetSelector.h"

#include "application/Text.h"

namespace
{
bool isZip(const Asset &asset)
{
    return endsWithCi(asset.name, ".zip");
}

bool isExe(const Asset &asset)
{
    return endsWithCi(asset.name, ".exe");
}
}

std::optional<Asset> selectReleaseAsset(const std::vector<Asset> &assets,
                                        const std::string &assetContains,
                                        const std::string &executablePath)
{
    if (!trimmed(assetContains).empty()) {
        for (const Asset &asset : assets) {
            if (containsCi(asset.name, assetContains) && (isZip(asset) || isExe(asset)))
                return asset;
        }
        return std::nullopt;
    }

    std::vector<Asset> zips;
    for (const Asset &asset : assets) {
        if (isZip(asset))
            zips.push_back(asset);
    }

    const std::string stem = fileStemOf(executablePath);
    if (!zips.empty()) {
        if (!stem.empty()) {
            for (const Asset &asset : zips) {
                if (containsCi(asset.name, stem))
                    return asset;
            }
        }
        for (const Asset &asset : zips) {
            if (containsCi(asset.name, "win"))
                return asset;
        }
        return zips.front();
    }

    for (const Asset &asset : assets) {
        if (isExe(asset))
            return asset;
    }
    return std::nullopt;
}
