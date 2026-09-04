// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#include "library/stems/stemsettings.h"

namespace mixxx {
namespace stems {

const QString kConfigGroup = QStringLiteral("[Stems]");

const ConfigKey kPythonExecutableConfigKey(kConfigGroup, QStringLiteral("python_executable"));
const ConfigKey kDemucsDeviceConfigKey(kConfigGroup, QStringLiteral("demucs_device"));
const ConfigKey kDemucsModelConfigKey(kConfigGroup, QStringLiteral("demucs_model"));
const ConfigKey kSaveInOriginalDirectoryConfigKey(kConfigGroup, QStringLiteral("save_in_original_directory"));
const ConfigKey kCustomSaveDirectoryConfigKey(kConfigGroup, QStringLiteral("custom_save_directory"));
const ConfigKey kAutoLoadToDeckConfigKey(kConfigGroup, QStringLiteral("auto_load_to_deck"));
const ConfigKey kFfmpegPathConfigKey(kConfigGroup, QStringLiteral("ffmpeg_path"));

StemSettings::StemSettings(UserSettingsPointer pConfig)
        : m_pConfig(pConfig) {
}

QString StemSettings::getPythonExecutable() const {
    if (!m_pConfig) {
        return QString();
    }
    return m_pConfig->getValue<QString>(kPythonExecutableConfigKey, QString());
}

void StemSettings::setPythonExecutable(const QString& path) {
    if (m_pConfig) {
        m_pConfig->setValue(kPythonExecutableConfigKey, path);
    }
}

QString StemSettings::getDemucsDevice() const {
    if (!m_pConfig) {
        return kDefaultDemucsDevice;
    }
    return m_pConfig->getValue<QString>(kDemucsDeviceConfigKey, kDefaultDemucsDevice);
}

void StemSettings::setDemucsDevice(const QString& device) {
    if (m_pConfig) {
        m_pConfig->setValue(kDemucsDeviceConfigKey, device);
    }
}

QString StemSettings::getDemucsModel() const {
    if (!m_pConfig) {
        return kDefaultDemucsModel;
    }
    return m_pConfig->getValue<QString>(kDemucsModelConfigKey, kDefaultDemucsModel);
}

void StemSettings::setDemucsModel(const QString& model) {
    if (m_pConfig) {
        m_pConfig->setValue(kDemucsModelConfigKey, model);
    }
}

bool StemSettings::getSaveInOriginalDirectory() const {
    if (!m_pConfig) {
        return kDefaultSaveInOriginalDirectory;
    }
    return m_pConfig->getValue<bool>(kSaveInOriginalDirectoryConfigKey, kDefaultSaveInOriginalDirectory);
}

void StemSettings::setSaveInOriginalDirectory(bool saveInOriginal) {
    if (m_pConfig) {
        m_pConfig->setValue(kSaveInOriginalDirectoryConfigKey, saveInOriginal);
    }
}

QString StemSettings::getDefaultStemDirectory() {
    QString musicPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (musicPath.isEmpty()) {
        musicPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QDir musicDir(musicPath);
    return musicDir.filePath(QStringLiteral("Mixxx/Stems"));
}

QString StemSettings::getCustomSaveDirectory() const {
    if (!m_pConfig) {
        return getDefaultStemDirectory();
    }
    QString dir = m_pConfig->getValue<QString>(kCustomSaveDirectoryConfigKey, QString());
    if (dir.trimmed().isEmpty()) {
        return getDefaultStemDirectory();
    }
    return dir;
}

void StemSettings::setCustomSaveDirectory(const QString& path) {
    if (m_pConfig) {
        m_pConfig->setValue(kCustomSaveDirectoryConfigKey, path);
    }
}

bool StemSettings::getAutoLoadToDeck() const {
    if (!m_pConfig) {
        return kDefaultAutoLoadToDeck;
    }
    return m_pConfig->getValue<bool>(kAutoLoadToDeckConfigKey, kDefaultAutoLoadToDeck);
}

void StemSettings::setAutoLoadToDeck(bool autoLoad) {
    if (m_pConfig) {
        m_pConfig->setValue(kAutoLoadToDeckConfigKey, autoLoad);
    }
}

QString StemSettings::getFfmpegPath() const {
    if (!m_pConfig) {
        return QString();
    }
    return m_pConfig->getValue<QString>(kFfmpegPathConfigKey, QString());
}

void StemSettings::setFfmpegPath(const QString& path) {
    if (m_pConfig) {
        m_pConfig->setValue(kFfmpegPathConfigKey, path);
    }
}

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
