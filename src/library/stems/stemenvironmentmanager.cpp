// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#include "library/stems/stemenvironmentmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include "moc_stemenvironmentmanager.cpp"
#include "util/logger.h"

namespace mixxx {
namespace stems {

namespace {
const Logger kLogger("StemEnvironmentManager");
} // anonymous namespace

StemEnvironmentManager::StemEnvironmentManager(StemSettings* pSettings, QObject* pParent)
        : QObject(pParent),
          m_pSettings(pSettings) {
}

StemEnvironmentManager::~StemEnvironmentManager() {
    cancelInstall();
}

QString StemEnvironmentManager::getRuntimeDirectory() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) {
        appData = QDir::homePath() + QStringLiteral("/.mixxx");
    }
    QDir dir(appData);
    return dir.filePath(QStringLiteral("stems-runtime"));
}

QString StemEnvironmentManager::findPythonBinary() {
    if (m_pSettings) {
        QString custom = m_pSettings->getPythonExecutable().trimmed();
        if (!custom.isEmpty() && QFileInfo::exists(custom)) {
            return custom;
        }
    }

    // Check isolated runtime environment inside AppData
    QString runtimeDir = getRuntimeDirectory();
#ifdef Q_OS_WIN
    QString runtimePython = runtimeDir + QStringLiteral("/Scripts/python.exe");
    if (QFileInfo::exists(runtimePython)) {
        return runtimePython;
    }
    runtimePython = runtimeDir + QStringLiteral("/python.exe");
    if (QFileInfo::exists(runtimePython)) {
        return runtimePython;
    }
#else
    QString runtimePython = runtimeDir + QStringLiteral("/bin/python3");
    if (QFileInfo::exists(runtimePython)) {
        return runtimePython;
    }
    runtimePython = runtimeDir + QStringLiteral("/bin/python");
    if (QFileInfo::exists(runtimePython)) {
        return runtimePython;
    }
#endif

#ifdef Q_OS_WIN
    // Check standard Windows Python installations
    QString localApp = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QStringList candidates = {
            QStringLiteral("C:/Program Files/Python311/python.exe"),
            QStringLiteral("C:/Program Files/Python312/python.exe"),
            QStringLiteral("C:/Program Files/Python310/python.exe"),
            QStringLiteral("C:/Program Files/Python313/python.exe"),
            localApp + QStringLiteral("/Programs/Python/Python311/python.exe"),
            localApp + QStringLiteral("/Programs/Python/Python312/python.exe"),
            localApp + QStringLiteral("/Programs/Python/Python310/python.exe"),
            QStringLiteral("C:/Python311/python.exe"),
            QStringLiteral("C:/Python312/python.exe"),
            QStringLiteral("C:/Python310/python.exe"),
    };
    for (const auto& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
#endif

    // Check system PATH (avoid WindowsApps store stub)
    QString inPath = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (!inPath.isEmpty() && !inPath.contains(QStringLiteral("WindowsApps"), Qt::CaseInsensitive)) {
        return inPath;
    }
    inPath = QStandardPaths::findExecutable(QStringLiteral("python"));
    if (!inPath.isEmpty() && !inPath.contains(QStringLiteral("WindowsApps"), Qt::CaseInsensitive)) {
        return inPath;
    }

    return QString();
}

QString StemEnvironmentManager::findFfmpegBinary() {
    if (m_pSettings) {
        QString custom = m_pSettings->getFfmpegPath().trimmed();
        if (!custom.isEmpty() && QFileInfo::exists(custom)) {
            return custom;
        }
    }

    // Check runtime dir
    QString runtimeDir = getRuntimeDirectory();
#ifdef Q_OS_WIN
    QString runtimeFfmpeg = runtimeDir + QStringLiteral("/ffmpeg.exe");
    if (QFileInfo::exists(runtimeFfmpeg)) {
        return runtimeFfmpeg;
    }
    runtimeFfmpeg = runtimeDir + QStringLiteral("/Scripts/ffmpeg.exe");
    if (QFileInfo::exists(runtimeFfmpeg)) {
        return runtimeFfmpeg;
    }
#else
    QString runtimeFfmpeg = runtimeDir + QStringLiteral("/bin/ffmpeg");
    if (QFileInfo::exists(runtimeFfmpeg)) {
        return runtimeFfmpeg;
    }
#endif

    // Check system PATH
    QString inPath = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (!inPath.isEmpty()) {
        return inPath;
    }

    // Check application directory
    QString appFfmpeg = QCoreApplication::applicationDirPath() +
#ifdef Q_OS_WIN
            QStringLiteral("/ffmpeg.exe");
#else
            QStringLiteral("/ffmpeg");
#endif
    if (QFileInfo::exists(appFfmpeg)) {
        return appFfmpeg;
    }

    return QString();
}

EnvironmentStatus StemEnvironmentManager::checkStatus() {
    EnvironmentStatus status;
    status.pythonPath = findPythonBinary();
    status.ffmpegPath = findFfmpegBinary();
    status.ffmpegAvailable = !status.ffmpegPath.isEmpty();

    if (status.pythonPath.isEmpty()) {
        return status;
    }

    // Check python version
    QProcess pyProc;
    pyProc.start(status.pythonPath, {QStringLiteral("--version")});
    if (pyProc.waitForFinished(6000)) {
        status.pythonVersion = QString::fromUtf8(pyProc.readAllStandardOutput()).trimmed();
        if (status.pythonVersion.isEmpty()) {
            status.pythonVersion = QString::fromUtf8(pyProc.readAllStandardError()).trimmed();
        }
        status.pythonAvailable = !status.pythonVersion.isEmpty();
    }

    if (!status.pythonAvailable) {
        return status;
    }

    // Check Demucs
    QProcess demucsProc;
    demucsProc.start(status.pythonPath, {QStringLiteral("-c"), QStringLiteral("import demucs; print(getattr(demucs, '__version__', 'ok'))")});
    if (demucsProc.waitForFinished(10000) && demucsProc.exitCode() == 0) {
        status.demucsVersion = QString::fromUtf8(demucsProc.readAllStandardOutput()).trimmed();
        status.demucsAvailable = true;
    }

    // Check PyTorch & CUDA
    QProcess torchProc;
    torchProc.start(status.pythonPath, {QStringLiteral("-c"), QStringLiteral("import torch; print(f'{torch.__version__}|{torch.cuda.is_available()}')")});
    if (torchProc.waitForFinished(10000) && torchProc.exitCode() == 0) {
        QString out = QString::fromUtf8(torchProc.readAllStandardOutput()).trimmed();
        QStringList parts = out.split(QLatin1Char('|'));
        if (!parts.isEmpty()) {
            status.torchVersion = parts[0];
            if (parts.size() > 1 && parts[1].trimmed() == QStringLiteral("True")) {
                status.cudaAvailable = true;
            }
        }
    }

    return status;
}

void StemEnvironmentManager::startEnvironmentInstall() {
    if (m_isInstalling) {
        return;
    }

    QString systemPython = findPythonBinary();
    if (systemPython.isEmpty()) {
        emit installFinished(false, tr("Python no encontrado en el sistema. Por favor instala Python 3.9+ primero."));
        return;
    }

    QString runtimeDir = getRuntimeDirectory();
    QDir().mkpath(runtimeDir);

    m_isInstalling = true;
    m_installStep = 1;
    emit installProgress(10, tr("Creando entorno virtual aislado en %1...").arg(runtimeDir));

    m_pInstallProcess = std::make_unique<QProcess>();
    connect(m_pInstallProcess.get(), &QProcess::readyReadStandardOutput, this, &StemEnvironmentManager::slotInstallProcessOutput);
    connect(m_pInstallProcess.get(), &QProcess::readyReadStandardError, this, &StemEnvironmentManager::slotInstallProcessOutput);
    connect(m_pInstallProcess.get(), &QProcess::finished, this, &StemEnvironmentManager::slotInstallProcessFinished);
    connect(m_pInstallProcess.get(), &QProcess::errorOccurred, this, &StemEnvironmentManager::slotInstallProcessError);

    // Step 1: Create venv
    m_pInstallProcess->start(systemPython, {QStringLiteral("-m"), QStringLiteral("venv"), runtimeDir});
}

void StemEnvironmentManager::cancelInstall() {
    if (m_pInstallProcess && m_pInstallProcess->state() != QProcess::NotRunning) {
        m_pInstallProcess->kill();
        m_pInstallProcess->waitForFinished(2000);
    }
    m_isInstalling = false;
    m_installStep = 0;
}

void StemEnvironmentManager::slotInstallProcessOutput() {
    if (!m_pInstallProcess) {
        return;
    }
    QString text = QString::fromUtf8(m_pInstallProcess->readAllStandardOutput());
    text += QString::fromUtf8(m_pInstallProcess->readAllStandardError());
    if (!text.trimmed().isEmpty()) {
        kLogger.debug() << "[Install]" << text.trimmed();
    }
}

void StemEnvironmentManager::slotInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    if (!m_isInstalling) {
        return;
    }

    if (exitCode != 0) {
        m_isInstalling = false;
        emit installFinished(false, tr("Falló la instalación en el paso %1 (código de error %2).").arg(m_installStep).arg(exitCode));
        return;
    }

    QString runtimeDir = getRuntimeDirectory();
#ifdef Q_OS_WIN
    QString pythonBin = runtimeDir + QStringLiteral("/Scripts/python.exe");
#else
    QString pythonBin = runtimeDir + QStringLiteral("/bin/python3");
#endif

    if (m_installStep == 1) {
        // Step 2: Install / Upgrade pip and wheel
        m_installStep = 2;
        emit installProgress(35, tr("Actualizando gestor de paquetes pip..."));
        m_pInstallProcess->start(pythonBin, {QStringLiteral("-m"), QStringLiteral("pip"), QStringLiteral("install"), QStringLiteral("--upgrade"), QStringLiteral("pip"), QStringLiteral("wheel"), QStringLiteral("setuptools")});
    } else if (m_installStep == 2) {
        // Step 3: Install Demucs
        m_installStep = 3;
        emit installProgress(60, tr("Descargando e instalando Demucs v4 y modelos de IA (esto puede tardar unos minutos)..."));
        m_pInstallProcess->start(pythonBin, {QStringLiteral("-m"), QStringLiteral("pip"), QStringLiteral("install"), QStringLiteral("demucs")});
    } else if (m_installStep == 3) {
        m_isInstalling = false;
        m_installStep = 0;
        if (m_pSettings) {
            m_pSettings->setPythonExecutable(pythonBin);
        }
        emit installProgress(100, tr("¡Entorno de IA configurado con éxito!"));
        emit installFinished(true, tr("Demucs y las librerías de IA se han instalado correctamente en el entorno de Mixxx."));
    }
}

void StemEnvironmentManager::slotInstallProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    if (m_isInstalling) {
        m_isInstalling = false;
        m_installStep = 0;
        emit installFinished(false, tr("Ocurrió un error al ejecutar el proceso del instalador."));
    }
}

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
