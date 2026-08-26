#include "library/ytdlp/ytdlpsettings.h"

namespace mixxx {
namespace ytdlp {

const QString kConfigGroup = QStringLiteral("[YtDlp]");

const ConfigKey kExecutablePathConfigKey(kConfigGroup, QStringLiteral("executable_path"));
const ConfigKey kDownloadDirectoryConfigKey(kConfigGroup, QStringLiteral("download_directory"));
const ConfigKey kAudioFormatConfigKey(kConfigGroup, QStringLiteral("audio_format"));
const ConfigKey kAudioQualityConfigKey(kConfigGroup, QStringLiteral("audio_quality"));
const ConfigKey kEmbedMetadataConfigKey(kConfigGroup, QStringLiteral("embed_metadata"));
const ConfigKey kEmbedThumbnailConfigKey(kConfigGroup, QStringLiteral("embed_thumbnail"));
const ConfigKey kAutoAnalyzeConfigKey(kConfigGroup, QStringLiteral("auto_analyze"));
const ConfigKey kCookiesBrowserConfigKey(kConfigGroup, QStringLiteral("cookies_browser"));

YtDlpSettings::YtDlpSettings(UserSettingsPointer pConfig)
        : m_pConfig(pConfig) {
}

QString YtDlpSettings::getExecutablePath() const {
    if (!m_pConfig) {
        return QString();
    }
    return m_pConfig->getValue<QString>(kExecutablePathConfigKey, QString());
}

void YtDlpSettings::setExecutablePath(const QString& path) {
    if (m_pConfig) {
        m_pConfig->setValue(kExecutablePathConfigKey, path);
    }
}

QString YtDlpSettings::getDefaultDownloadDirectory() {
    QString musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (musicPath.isEmpty()) {
        musicPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QDir musicDir(musicPath);
    return musicDir.filePath(QStringLiteral("Mixxx/Downloads"));
}

QString YtDlpSettings::getDownloadDirectory() const {
    if (!m_pConfig) {
        return getDefaultDownloadDirectory();
    }
    QString dir = m_pConfig->getValue<QString>(kDownloadDirectoryConfigKey, QString());
    if (dir.trimmed().isEmpty()) {
        return getDefaultDownloadDirectory();
    }
    return dir;
}

void YtDlpSettings::setDownloadDirectory(const QString& path) {
    if (m_pConfig) {
        m_pConfig->setValue(kDownloadDirectoryConfigKey, path);
    }
}

QString YtDlpSettings::getAudioFormat() const {
    if (!m_pConfig) {
        return kDefaultAudioFormat;
    }
    return m_pConfig->getValue<QString>(kAudioFormatConfigKey, kDefaultAudioFormat);
}

void YtDlpSettings::setAudioFormat(const QString& format) {
    if (m_pConfig) {
        m_pConfig->setValue(kAudioFormatConfigKey, format);
    }
}

QString YtDlpSettings::getAudioQuality() const {
    if (!m_pConfig) {
        return kDefaultAudioQuality;
    }
    return m_pConfig->getValue<QString>(kAudioQualityConfigKey, kDefaultAudioQuality);
}

void YtDlpSettings::setAudioQuality(const QString& quality) {
    if (m_pConfig) {
        m_pConfig->setValue(kAudioQualityConfigKey, quality);
    }
}

bool YtDlpSettings::getEmbedMetadata() const {
    if (!m_pConfig) {
        return kDefaultEmbedMetadata;
    }
    return m_pConfig->getValue<bool>(kEmbedMetadataConfigKey, kDefaultEmbedMetadata);
}

void YtDlpSettings::setEmbedMetadata(bool embed) {
    if (m_pConfig) {
        m_pConfig->setValue(kEmbedMetadataConfigKey, embed);
    }
}

bool YtDlpSettings::getEmbedThumbnail() const {
    if (!m_pConfig) {
        return kDefaultEmbedThumbnail;
    }
    return m_pConfig->getValue<bool>(kEmbedThumbnailConfigKey, kDefaultEmbedThumbnail);
}

void YtDlpSettings::setEmbedThumbnail(bool embed) {
    if (m_pConfig) {
        m_pConfig->setValue(kEmbedThumbnailConfigKey, embed);
    }
}

bool YtDlpSettings::getAutoAnalyze() const {
    if (!m_pConfig) {
        return kDefaultAutoAnalyze;
    }
    return m_pConfig->getValue<bool>(kAutoAnalyzeConfigKey, kDefaultAutoAnalyze);
}

void YtDlpSettings::setAutoAnalyze(bool autoAnalyze) {
    if (m_pConfig) {
        m_pConfig->setValue(kAutoAnalyzeConfigKey, autoAnalyze);
    }
}

QString YtDlpSettings::getCookiesBrowser() const {
    if (!m_pConfig) {
        return QString();
    }
    return m_pConfig->getValue<QString>(kCookiesBrowserConfigKey, QString());
}

void YtDlpSettings::setCookiesBrowser(const QString& browser) {
    if (m_pConfig) {
        m_pConfig->setValue(kCookiesBrowserConfigKey, browser);
    }
}

} // namespace ytdlp
} // namespace mixxx
