// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#pragma once

#include <QDir>
#include <QString>
#include <QStandardPaths>

#include "preferences/configobject.h"
#include "preferences/usersettings.h"

namespace mixxx {
namespace stems {

extern const QString kConfigGroup;

extern const ConfigKey kPythonExecutableConfigKey;
extern const ConfigKey kDemucsDeviceConfigKey;
extern const ConfigKey kDemucsModelConfigKey;
extern const ConfigKey kSaveInOriginalDirectoryConfigKey;
extern const ConfigKey kCustomSaveDirectoryConfigKey;
extern const ConfigKey kAutoLoadToDeckConfigKey;
extern const ConfigKey kFfmpegPathConfigKey;

const QString kDefaultDemucsDevice = QStringLiteral("auto");
const QString kDefaultDemucsModel = QStringLiteral("htdemucs");
const bool kDefaultSaveInOriginalDirectory = true;
const bool kDefaultAutoLoadToDeck = true;

class StemSettings {
  public:
    explicit StemSettings(UserSettingsPointer pConfig);

    QString getPythonExecutable() const;
    void setPythonExecutable(const QString& path);

    QString getDemucsDevice() const;
    void setDemucsDevice(const QString& device);

    QString getDemucsModel() const;
    void setDemucsModel(const QString& model);

    bool getSaveInOriginalDirectory() const;
    void setSaveInOriginalDirectory(bool saveInOriginal);

    QString getCustomSaveDirectory() const;
    void setCustomSaveDirectory(const QString& path);

    bool getAutoLoadToDeck() const;
    void setAutoLoadToDeck(bool autoLoad);

    QString getFfmpegPath() const;
    void setFfmpegPath(const QString& path);

    static QString getDefaultStemDirectory();

  private:
    UserSettingsPointer m_pConfig;
};

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
