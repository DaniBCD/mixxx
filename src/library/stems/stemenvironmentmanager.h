// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <memory>

#include "library/stems/stemsettings.h"

namespace mixxx {
namespace stems {

struct EnvironmentStatus {
    bool pythonAvailable = false;
    QString pythonPath;
    QString pythonVersion;

    bool demucsAvailable = false;
    QString demucsVersion;

    bool ffmpegAvailable = false;
    QString ffmpegPath;

    bool cudaAvailable = false;
    QString torchVersion;

    bool isReady() const {
        return pythonAvailable && demucsAvailable && ffmpegAvailable;
    }
};

class StemEnvironmentManager : public QObject {
    Q_OBJECT

  public:
    explicit StemEnvironmentManager(StemSettings* pSettings, QObject* pParent = nullptr);
    ~StemEnvironmentManager() override;

    /// Checks the current status of all dependencies (Python, Demucs, FFmpeg, CUDA)
    EnvironmentStatus checkStatus();

    /// Returns the path to the portable runtime directory in app data
    static QString getRuntimeDirectory();

    /// Starts automated installation/setup of Demucs & dependencies in runtime dir
    void startEnvironmentInstall();

    /// Cancels an in-progress installation
    void cancelInstall();

    bool isInstalling() const {
        return m_isInstalling;
    }

  signals:
    void installProgress(int percent, const QString& status);
    void installFinished(bool success, const QString& message);

  private slots:
    void slotInstallProcessOutput();
    void slotInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotInstallProcessError(QProcess::ProcessError error);

  private:
    QString findPythonBinary();
    QString findFfmpegBinary();
    void runPipInstall(const QString& pythonBin, const QStringList& packages);

    StemSettings* m_pSettings;
    std::unique_ptr<QProcess> m_pInstallProcess;
    bool m_isInstalling = false;
    int m_installStep = 0;
};

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
