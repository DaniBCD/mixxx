// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#include "preferences/dialog/dlgprefstems.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "moc_dlgprefstems.cpp"

DlgPrefStems::DlgPrefStems(QWidget* pParent, UserSettingsPointer pConfig)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_settings(pConfig),
          m_envManager(&m_settings, this) {
    setupUi(this);

    comboDevice->clear();
    comboDevice->addItem(tr("Automático (Recomendado)"), QStringLiteral("auto"));
    comboDevice->addItem(tr("GPU (NVIDIA CUDA)"), QStringLiteral("cuda"));
    comboDevice->addItem(tr("CPU (Procesador principal)"), QStringLiteral("cpu"));

    comboModel->clear();
    comboModel->addItem(tr("HTDemucs v4 (Rápido y alta calidad)"), QStringLiteral("htdemucs"));
    comboModel->addItem(tr("HTDemucs Fine-Tuned (Máxima fidelidad)"), QStringLiteral("htdemucs_ft"));

    connect(btnCheckStatus, &QPushButton::clicked, this, &DlgPrefStems::slotCheckStatus);
    connect(btnInstallEnv, &QPushButton::clicked, this, &DlgPrefStems::slotInstallEnv);
    connect(btnBrowseDir, &QPushButton::clicked, this, &DlgPrefStems::slotBrowseCustomDir);
    connect(btnBrowsePython, &QPushButton::clicked, this, &DlgPrefStems::slotBrowsePython);
    connect(btnBrowseFfmpeg, &QPushButton::clicked, this, &DlgPrefStems::slotBrowseFfmpeg);

    connect(radioSameDir, &QRadioButton::toggled, this, &DlgPrefStems::slotRadioStorageToggled);
    connect(radioCustomDir, &QRadioButton::toggled, this, &DlgPrefStems::slotRadioStorageToggled);

    connect(&m_envManager, &mixxx::stems::StemEnvironmentManager::installProgress,
            this, &DlgPrefStems::slotInstallProgress);
    connect(&m_envManager, &mixxx::stems::StemEnvironmentManager::installFinished,
            this, &DlgPrefStems::slotInstallFinished);

    slotUpdate();
}

void DlgPrefStems::slotUpdate() {
    QString device = m_settings.getDemucsDevice();
    int devIdx = comboDevice->findData(device);
    if (devIdx >= 0) {
        comboDevice->setCurrentIndex(devIdx);
    }

    QString model = m_settings.getDemucsModel();
    int modelIdx = comboModel->findData(model);
    if (modelIdx >= 0) {
        comboModel->setCurrentIndex(modelIdx);
    }

    checkAutoLoad->setChecked(m_settings.getAutoLoadToDeck());

    bool sameDir = m_settings.getSaveInOriginalDirectory();
    radioSameDir->setChecked(sameDir);
    radioCustomDir->setChecked(!sameDir);
    lineEditCustomDir->setText(m_settings.getCustomSaveDirectory());
    slotRadioStorageToggled();

    lineEditPython->setText(m_settings.getPythonExecutable());
    lineEditFfmpeg->setText(m_settings.getFfmpegPath());

    slotCheckStatus();
}

void DlgPrefStems::slotApply() {
    m_settings.setDemucsDevice(comboDevice->currentData().toString());
    m_settings.setDemucsModel(comboModel->currentData().toString());
    m_settings.setAutoLoadToDeck(checkAutoLoad->isChecked());

    m_settings.setSaveInOriginalDirectory(radioSameDir->isChecked());
    m_settings.setCustomSaveDirectory(lineEditCustomDir->text().trimmed());

    m_settings.setPythonExecutable(lineEditPython->text().trimmed());
    m_settings.setFfmpegPath(lineEditFfmpeg->text().trimmed());
}

void DlgPrefStems::slotResetToDefaults() {
    int devIdx = comboDevice->findData(mixxx::stems::kDefaultDemucsDevice);
    if (devIdx >= 0) {
        comboDevice->setCurrentIndex(devIdx);
    }

    int modelIdx = comboModel->findData(mixxx::stems::kDefaultDemucsModel);
    if (modelIdx >= 0) {
        comboModel->setCurrentIndex(modelIdx);
    }

    checkAutoLoad->setChecked(mixxx::stems::kDefaultAutoLoadToDeck);

    radioSameDir->setChecked(mixxx::stems::kDefaultSaveInOriginalDirectory);
    radioCustomDir->setChecked(!mixxx::stems::kDefaultSaveInOriginalDirectory);
    lineEditCustomDir->setText(mixxx::stems::StemSettings::getDefaultStemDirectory());
    slotRadioStorageToggled();

    lineEditPython->clear();
    lineEditFfmpeg->clear();

    slotCheckStatus();
}

void DlgPrefStems::slotRadioStorageToggled() {
    bool custom = radioCustomDir->isChecked();
    lineEditCustomDir->setEnabled(custom);
    btnBrowseDir->setEnabled(custom);
}

void DlgPrefStems::slotCheckStatus() {
    mixxx::stems::EnvironmentStatus status = m_envManager.checkStatus();
    QString text;

    if (status.isReady()) {
        text += QStringLiteral("✅ <b>Estado: LISTO PARA SEPARAR STEMS</b><br>");
    } else {
        text += QStringLiteral("⚠️ <b>Estado: Componentes incompletos o no instalados</b><br>");
    }

    text += tr("• Python: %1 (%2)<br>")
            .arg(status.pythonAvailable ? QStringLiteral("Detectado") : QStringLiteral("No encontrado"))
            .arg(status.pythonVersion.isEmpty() ? status.pythonPath : status.pythonVersion);

    text += tr("• Demucs: %1 (%2)<br>")
            .arg(status.demucsAvailable ? QStringLiteral("Instalado") : QStringLiteral("No instalado"))
            .arg(status.demucsVersion.isEmpty() ? tr("Desconocido") : status.demucsVersion);

    text += tr("• FFmpeg: %1 (%2)<br>")
            .arg(status.ffmpegAvailable ? QStringLiteral("Detectado") : QStringLiteral("No encontrado"))
            .arg(status.ffmpegPath.isEmpty() ? tr("No disponible") : status.ffmpegPath);

    text += tr("• Aceleración GPU: %1 (%2)")
            .arg(status.cudaAvailable ? QStringLiteral("GPU CUDA Activada 🚀") : QStringLiteral("CPU únicamente"))
            .arg(status.torchVersion.isEmpty() ? tr("PyTorch no detectado") : tr("PyTorch %1").arg(status.torchVersion));

    labelEnvStatus->setText(text);
}

void DlgPrefStems::slotInstallEnv() {
    btnInstallEnv->setEnabled(false);
    progressInstall->setEnabled(true);
    progressInstall->setValue(0);
    labelInstallStatus->setText(tr("Iniciando preparación del entorno..."));
    m_envManager.startEnvironmentInstall();
}

void DlgPrefStems::slotInstallProgress(int percent, const QString& status) {
    progressInstall->setValue(percent);
    labelInstallStatus->setText(status);
}

void DlgPrefStems::slotInstallFinished(bool success, const QString& message) {
    btnInstallEnv->setEnabled(true);
    if (success) {
        progressInstall->setValue(100);
        labelInstallStatus->setText(tr("Instalación completada."));
        QMessageBox::information(this, tr("Entorno de IA"), message);
    } else {
        labelInstallStatus->setText(tr("Error en la instalación."));
        QMessageBox::warning(this, tr("Entorno de IA"), message);
    }
    slotCheckStatus();
}

void DlgPrefStems::slotBrowseCustomDir() {
    QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Seleccionar carpeta para Stems"),
            lineEditCustomDir->text());
    if (!dir.isEmpty()) {
        lineEditCustomDir->setText(dir);
    }
}

void DlgPrefStems::slotBrowsePython() {
    QString filter =
#ifdef Q_OS_WIN
            tr("Ejecutable de Python (python.exe);;Todos los archivos (*.*)");
#else
            tr("Ejecutable de Python (python*);;Todos los archivos (*)");
#endif
    QString file = QFileDialog::getOpenFileName(
            this,
            tr("Seleccionar ejecutable de Python"),
            lineEditPython->text(),
            filter);
    if (!file.isEmpty()) {
        lineEditPython->setText(file);
        slotCheckStatus();
    }
}

void DlgPrefStems::slotBrowseFfmpeg() {
    QString filter =
#ifdef Q_OS_WIN
            tr("Ejecutable de FFmpeg (ffmpeg.exe);;Todos los archivos (*.*)");
#else
            tr("Ejecutable de FFmpeg (ffmpeg*);;Todos los archivos (*)");
#endif
    QString file = QFileDialog::getOpenFileName(
            this,
            tr("Seleccionar ejecutable de FFmpeg"),
            lineEditFfmpeg->text(),
            filter);
    if (!file.isEmpty()) {
        lineEditFfmpeg->setText(file);
        slotCheckStatus();
    }
}
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
