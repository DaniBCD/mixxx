#pragma once

#include <QDialog>

#include "library/ytdlp/ui_dlgdownloadurl.h"
#include "library/ytdlp/ytdlpservice.h"
#include "preferences/usersettings.h"

namespace mixxx {
namespace ytdlp {

class DlgDownloadUrl : public QDialog, public Ui::DlgDownloadUrl {
    Q_OBJECT

  public:
    DlgDownloadUrl(
            YtDlpService* pService,
            QWidget* pParent = nullptr);
    ~DlgDownloadUrl() override = default;

    void setUrl(const QUrl& url);
    void setTargetDeck(const QString& targetDeckGroup);

  public slots:
    void slotPasteUrl();
    void slotStartDownload();
    void slotCancelDownload();

  private slots:
    void slotJobStarted(const QString& jobId, const QUrl& url, const QString& targetDeckGroup);
    void slotJobProgress(const QString& jobId, double percent, const QString& speed, const QString& eta);
    void slotJobFinished(const QString& jobId, const QString& filePath, const QString& targetDeckGroup, TrackPointer pTrack);
    void slotJobFailed(const QString& jobId, const QString& errorMessage);

  private:
    void initDecks();
    void initFormats();

    YtDlpService* m_pService;
    QString m_activeJobId;
};

} // namespace ytdlp
} // namespace mixxx
