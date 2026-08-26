#include "library/ytdlp/dlgdownloadurl.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>

#include "moc_dlgdownloadurl.cpp"

namespace mixxx {
namespace ytdlp {

DlgDownloadUrl::DlgDownloadUrl(
        YtDlpService* pService,
        QWidget* pParent)
        : QDialog(pParent),
          m_pService(pService) {
    setupUi(this);

    initDecks();
    initFormats();

    connect(btnPaste, &QPushButton::clicked, this, &DlgDownloadUrl::slotPasteUrl);
    connect(btnDownload, &QPushButton::clicked, this, &DlgDownloadUrl::slotStartDownload);
    connect(btnCancel, &QPushButton::clicked, this, &DlgDownloadUrl::slotCancelDownload);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    if (m_pService) {
        connect(m_pService, &YtDlpService::jobStarted, this, &DlgDownloadUrl::slotJobStarted);
        connect(m_pService, &YtDlpService::jobProgress, this, &DlgDownloadUrl::slotJobProgress);
        connect(m_pService, &YtDlpService::jobFinished, this, &DlgDownloadUrl::slotJobFinished);
        connect(m_pService, &YtDlpService::jobFailed, this, &DlgDownloadUrl::slotJobFailed);
    }
}

void DlgDownloadUrl::initDecks() {
    comboTargetDeck->clear();
    comboTargetDeck->addItem(tr("Do not load (Add to Library only)"), QString());
    comboTargetDeck->addItem(tr("Deck 1"), QStringLiteral("[Channel1]"));
    comboTargetDeck->addItem(tr("Deck 2"), QStringLiteral("[Channel2]"));
    comboTargetDeck->addItem(tr("Deck 3"), QStringLiteral("[Channel3]"));
    comboTargetDeck->addItem(tr("Deck 4"), QStringLiteral("[Channel4]"));
    comboTargetDeck->addItem(tr("Preview Deck"), QStringLiteral("[PreviewDeck1]"));
}

void DlgDownloadUrl::initFormats() {
    comboAudioFormat->clear();
    comboAudioFormat->addItem(tr("FLAC (Lossless)"), QStringLiteral("flac"));
    comboAudioFormat->addItem(tr("MP3 (High Quality 320k)"), QStringLiteral("mp3"));
    comboAudioFormat->addItem(tr("Opus (Best Audio Stream)"), QStringLiteral("opus"));
    comboAudioFormat->addItem(tr("M4A / AAC"), QStringLiteral("m4a"));
    comboAudioFormat->addItem(tr("Ogg Vorbis"), QStringLiteral("vorbis"));

    if (m_pService) {
        QString defaultFormat = m_pService->settings().getAudioFormat();
        int idx = comboAudioFormat->findData(defaultFormat);
        if (idx >= 0) {
            comboAudioFormat->setCurrentIndex(idx);
        }
    }
}

void DlgDownloadUrl::setUrl(const QUrl& url) {
    lineEditUrl->setText(url.toString());
}

void DlgDownloadUrl::setTargetDeck(const QString& targetDeckGroup) {
    int idx = comboTargetDeck->findData(targetDeckGroup);
    if (idx >= 0) {
        comboTargetDeck->setCurrentIndex(idx);
    }
}

void DlgDownloadUrl::slotPasteUrl() {
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        QString text = clipboard->text().trimmed();
        if (!text.isEmpty()) {
            lineEditUrl->setText(text);
        }
    }
}

void DlgDownloadUrl::slotStartDownload() {
    QString rawUrl = lineEditUrl->text().trimmed();
    if (rawUrl.isEmpty()) {
        QMessageBox::warning(this, tr("URL Missing"), tr("Please enter or paste a valid URL."));
        return;
    }

    QUrl url(rawUrl);
    if (!url.isValid() || url.scheme().isEmpty()) {
        url = QUrl(QStringLiteral("https://") + rawUrl);
    }

    if (!m_pService) {
        QMessageBox::critical(this, tr("Error"), tr("yt-dlp service is not initialized."));
        return;
    }

    QString targetDeck = comboTargetDeck->currentData().toString();
    QString format = comboAudioFormat->currentData().toString();

    btnDownload->setEnabled(false);
    btnCancel->setEnabled(true);
    progressBar->setValue(0);
    labelStatus->setText(tr("Starting download..."));

    m_activeJobId = m_pService->startDownload(url, targetDeck, format);
}

void DlgDownloadUrl::slotCancelDownload() {
    if (!m_activeJobId.isEmpty() && m_pService) {
        m_pService->cancelJob(m_activeJobId);
        labelStatus->setText(tr("Download cancelled."));
        btnDownload->setEnabled(true);
        btnCancel->setEnabled(false);
    }
}

void DlgDownloadUrl::slotJobStarted(
        const QString& jobId,
        const QUrl& url,
        const QString& targetDeckGroup) {
    Q_UNUSED(url);
    Q_UNUSED(targetDeckGroup);
    if (jobId == m_activeJobId) {
        labelStatus->setText(tr("Downloading audio..."));
    }
}

void DlgDownloadUrl::slotJobProgress(
        const QString& jobId,
        double percent,
        const QString& speed,
        const QString& eta) {
    if (jobId == m_activeJobId) {
        progressBar->setValue(static_cast<int>(percent));
        QString statusText = tr("%1%").arg(percent, 0, 'f', 1);
        if (!speed.isEmpty()) {
            statusText += QStringLiteral(" @ ") + speed;
        }
        if (!eta.isEmpty()) {
            statusText += tr(" (ETA: %1)").arg(eta);
        }
        labelStatus->setText(statusText);
    }
}

void DlgDownloadUrl::slotJobFinished(
        const QString& jobId,
        const QString& filePath,
        const QString& targetDeckGroup,
        TrackPointer pTrack) {
    Q_UNUSED(filePath);
    Q_UNUSED(targetDeckGroup);
    Q_UNUSED(pTrack);
    if (jobId == m_activeJobId) {
        progressBar->setValue(100);
        labelStatus->setText(tr("Download completed and imported into Mixxx!"));
        btnDownload->setEnabled(true);
        btnCancel->setEnabled(false);
    }
}

void DlgDownloadUrl::slotJobFailed(
        const QString& jobId,
        const QString& errorMessage) {
    if (jobId == m_activeJobId) {
        labelStatus->setText(tr("Error: %1").arg(errorMessage));
        btnDownload->setEnabled(true);
        btnCancel->setEnabled(false);
    }
}

} // namespace ytdlp
} // namespace mixxx
