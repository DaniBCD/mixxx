#include "preferences/dialog/dlgprefytdlp.h"

#include <QFileDialog>
#include <QFileInfo>

#include "moc_dlgprefytdlp.cpp"

DlgPrefYtDlp::DlgPrefYtDlp(QWidget* pParent, UserSettingsPointer pConfig)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_settings(pConfig) {
    setupUi(this);

    comboFormat->clear();
    comboFormat->addItem(tr("FLAC (Lossless)"), QStringLiteral("flac"));
    comboFormat->addItem(tr("MP3 (High Quality 320k)"), QStringLiteral("mp3"));
    comboFormat->addItem(tr("Opus (Best Stream Quality)"), QStringLiteral("opus"));
    comboFormat->addItem(tr("M4A / AAC"), QStringLiteral("m4a"));
    comboFormat->addItem(tr("Ogg Vorbis"), QStringLiteral("vorbis"));

    connect(btnBrowseExecutable, &QPushButton::clicked, this, &DlgPrefYtDlp::slotBrowseExecutable);
    connect(btnTestExecutable, &QPushButton::clicked, this, &DlgPrefYtDlp::slotTestExecutable);
    connect(btnBrowseDownloadDir, &QPushButton::clicked, this, &DlgPrefYtDlp::slotBrowseDownloadDir);

    slotUpdate();
}

void DlgPrefYtDlp::slotUpdate() {
    lineEditPath->setText(m_settings.getExecutablePath());
    lineEditDownloadDir->setText(m_settings.getDownloadDirectory());

    QString format = m_settings.getAudioFormat();
    int idx = comboFormat->findData(format);
    if (idx >= 0) {
        comboFormat->setCurrentIndex(idx);
    }

    checkEmbedMetadata->setChecked(m_settings.getEmbedMetadata());
    checkEmbedThumbnail->setChecked(m_settings.getEmbedThumbnail());
    checkAutoAnalyze->setChecked(m_settings.getAutoAnalyze());
    lineEditCookies->setText(m_settings.getCookiesBrowser());

    slotTestExecutable();
}

void DlgPrefYtDlp::slotApply() {
    m_settings.setExecutablePath(lineEditPath->text().trimmed());
    m_settings.setDownloadDirectory(lineEditDownloadDir->text().trimmed());
    m_settings.setAudioFormat(comboFormat->currentData().toString());
    m_settings.setEmbedMetadata(checkEmbedMetadata->isChecked());
    m_settings.setEmbedThumbnail(checkEmbedThumbnail->isChecked());
    m_settings.setAutoAnalyze(checkAutoAnalyze->isChecked());
    m_settings.setCookiesBrowser(lineEditCookies->text().trimmed());
}

void DlgPrefYtDlp::slotResetToDefaults() {
    lineEditPath->clear();
    lineEditDownloadDir->setText(mixxx::ytdlp::YtDlpSettings::getDefaultDownloadDirectory());
    int idx = comboFormat->findData(mixxx::ytdlp::kDefaultAudioFormat);
    if (idx >= 0) {
        comboFormat->setCurrentIndex(idx);
    }
    checkEmbedMetadata->setChecked(mixxx::ytdlp::kDefaultEmbedMetadata);
    checkEmbedThumbnail->setChecked(mixxx::ytdlp::kDefaultEmbedThumbnail);
    checkAutoAnalyze->setChecked(mixxx::ytdlp::kDefaultAutoAnalyze);
    lineEditCookies->clear();
}

void DlgPrefYtDlp::slotBrowseExecutable() {
    QString filter =
#ifdef Q_OS_WIN
            tr("Executable (*.exe);;All Files (*.*)");
#else
            tr("All Files (*)");
#endif
    QString path = QFileDialog::getOpenFileName(
            this,
            tr("Select yt-dlp Executable"),
            lineEditPath->text().trimmed(),
            filter);
    if (!path.isEmpty()) {
        lineEditPath->setText(path);
        slotTestExecutable();
    }
}

void DlgPrefYtDlp::slotTestExecutable() {
    QString customPath = lineEditPath->text().trimmed();
    QString resolved = mixxx::ytdlp::YtDlpService::findYtDlpBinary(customPath);
    if (resolved.isEmpty()) {
        labelVersionStatus->setText(tr("<font color='red'>yt-dlp was not found. Please install yt-dlp or specify its path.</font>"));
    } else {
        QString ver = mixxx::ytdlp::YtDlpService::getYtDlpVersion(customPath);
        if (!ver.isEmpty()) {
            labelVersionStatus->setText(tr("<font color='green'>Found yt-dlp (Version %1) at %2</font>").arg(ver, resolved));
        } else {
            labelVersionStatus->setText(tr("<font color='green'>Found binary at %1</font>").arg(resolved));
        }
    }
}

void DlgPrefYtDlp::slotBrowseDownloadDir() {
    QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Select Download Directory"),
            lineEditDownloadDir->text().trimmed());
    if (!dir.isEmpty()) {
        lineEditDownloadDir->setText(dir);
    }
}
