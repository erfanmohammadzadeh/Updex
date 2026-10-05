#pragma once

#include "domain/Release.h"

#include <optional>
#include <string>
#include <vector>

std::optional<Asset> selectReleaseAsset(const std::vector<Asset> &assets,
                                        const std::string &assetContains,
                                        const std::string &executablePath);
