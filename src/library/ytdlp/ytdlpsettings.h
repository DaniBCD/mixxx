#pragma once

#include <QString>
#include <QDir>
#include <QStandardPaths>

#include "preferences/configobject.h"
#include "preferences/usersettings.h"

namespace mixxx {
namespace ytdlp {

extern const QString kConfigGroup;

extern const ConfigKey kExecutablePathConfigKey;
extern const ConfigKey kDownloadDirectoryConfigKey;
extern const ConfigKey kAudioFormatConfigKey;
extern const ConfigKey kAudioQualityConfigKey;
extern const ConfigKey kEmbedMetadataConfigKey;
extern const ConfigKey kEmbedThumbnailConfigKey;
extern const ConfigKey kAutoAnalyzeConfigKey;
extern const ConfigKey kCookiesBrowserConfigKey;

const QString kDefaultAudioFormat = QStringLiteral("flac");
const QString kDefaultAudioQuality = QStringLiteral("0");
const bool kDefaultEmbedMetadata = true;
const bool kDefaultEmbedThumbnail = true;
const bool kDefaultAutoAnalyze = true;

class YtDlpSettings {
  public:
    explicit YtDlpSettings(UserSettingsPointer pConfig);

    QString getExecutablePath() const;
    void setExecutablePath(const QString& path);

    QString getDownloadDirectory() const;
    void setDownloadDirectory(const QString& path);

    QString getAudioFormat() const;
    void setAudioFormat(const QString& format);

    QString getAudioQuality() const;
    void setAudioQuality(const QString& quality);

    bool getEmbedMetadata() const;
    void setEmbedMetadata(bool embed);

    bool getEmbedThumbnail() const;
    void setEmbedThumbnail(bool embed);

    bool getAutoAnalyze() const;
    void setAutoAnalyze(bool autoAnalyze);

    QString getCookiesBrowser() const;
    void setCookiesBrowser(const QString& browser);

    static QString getDefaultDownloadDirectory();

  private:
    UserSettingsPointer m_pConfig;
};

} // namespace ytdlp
} // namespace mixxx
