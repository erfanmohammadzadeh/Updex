#pragma once

#include "application/ports/ISettingsStore.h"

#include <QString>

class JsonSettingsStore final : public ISettingsStore
{
public:
    explicit JsonSettingsStore(const QString &directory);

    bool load(UpdaterSettings &settings, std::string &error) const override;
    bool save(const UpdaterSettings &settings) override;
    std::string filePath() const override;

private:
    QString m_filePath;
};
