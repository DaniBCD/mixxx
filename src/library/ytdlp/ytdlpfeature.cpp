#include "library/ytdlp/ytdlpfeature.h"

#include "library/library.h"
#include "library/ytdlp/dlgdownloadurl.h"
#include "moc_ytdlpfeature.cpp"
#include "widget/wlibrary.h"

namespace mixxx {
namespace ytdlp {

namespace {
const QString kViewName = QStringLiteral("YtDlpDownloads");
} // anonymous namespace

YtDlpFeature::YtDlpFeature(
        Library* pLibrary,
        UserSettingsPointer pConfig,
        YtDlpService* pService)
        : LibraryFeature(pLibrary, pConfig, QStringLiteral("download")),
          m_baseTitle(tr("Online / Downloads")),
          m_title(m_baseTitle),
          m_pService(pService),
          m_pSidebarModel(make_parented<TreeItemModel>(this)) {
    if (m_pService) {
        connect(m_pService, &YtDlpService::jobStarted, this, &YtDlpFeature::slotJobStarted);
        connect(m_pService, &YtDlpService::jobProgress, this, &YtDlpFeature::slotJobProgress);
        connect(m_pService, &YtDlpService::jobFinished, this, &YtDlpFeature::slotJobFinished);
        connect(m_pService, &YtDlpService::jobFailed, this, &YtDlpFeature::slotJobFailed);
    }
}

TreeItemModel* YtDlpFeature::sidebarModel() const {
    return m_pSidebarModel;
}

void YtDlpFeature::bindLibraryWidget(
        WLibrary* libraryWidget,
        KeyboardEventFilter* keyboard) {
    Q_UNUSED(libraryWidget);
    Q_UNUSED(keyboard);
}

void YtDlpFeature::activate() {
    slotOpenDownloadDialog();
}

void YtDlpFeature::slotOpenDownloadDialog(const QUrl& url, const QString& targetDeck) {
    DlgDownloadUrl dlg(m_pService);
    if (url.isValid()) {
        dlg.setUrl(url);
    }
    if (!targetDeck.isEmpty()) {
        dlg.setTargetDeck(targetDeck);
    }
    dlg.exec();
}

bool YtDlpFeature::dropAccept(const QList<QUrl>& urls, QObject* pSource) {
    Q_UNUSED(pSource);
    for (const auto& url : urls) {
        QString scheme = url.scheme().toLower();
        if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
            if (m_pService) {
                m_pService->startDownload(url);
            }
            return true;
        }
    }
    return false;
}

bool YtDlpFeature::dragMoveAccept(const QList<QUrl>& urls) {
    for (const auto& url : urls) {
        QString scheme = url.scheme().toLower();
        if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
            return true;
        }
    }
    return false;
}

void YtDlpFeature::slotJobStarted(
        const QString& jobId,
        const QUrl& url,
        const QString& targetDeckGroup) {
    Q_UNUSED(jobId);
    Q_UNUSED(url);
    Q_UNUSED(targetDeckGroup);
    m_title = tr("%1 (Downloading...)").arg(m_baseTitle);
    emit featureIsLoading(this, false);
}

void YtDlpFeature::slotJobProgress(
        const QString& jobId,
        double percent,
        const QString& speed,
        const QString& eta) {
    Q_UNUSED(jobId);
    Q_UNUSED(speed);
    Q_UNUSED(eta);
    m_title = tr("%1 (%2%)").arg(m_baseTitle).arg(static_cast<int>(percent));
    emit featureIsLoading(this, false);
}

void YtDlpFeature::slotJobFinished(
        const QString& jobId,
        const QString& filePath,
        const QString& targetDeckGroup,
        TrackPointer pTrack) {
    Q_UNUSED(jobId);
    Q_UNUSED(filePath);
    Q_UNUSED(targetDeckGroup);
    Q_UNUSED(pTrack);
    m_title = m_baseTitle;
    emit featureIsLoading(this, false);
}

void YtDlpFeature::slotJobFailed(
        const QString& jobId,
        const QString& errorMessage) {
    Q_UNUSED(jobId);
    Q_UNUSED(errorMessage);
    m_title = m_baseTitle;
    emit featureIsLoading(this, false);
}

} // namespace ytdlp
} // namespace mixxx
