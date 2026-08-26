#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QUrl>
#include <memory>

#include "library/trackcollectionmanager.h"
#include "library/ytdlp/ytdlpsettings.h"
#include "track/track_decl.h"
#include "util/parented_ptr.h"

namespace mixxx {
namespace ytdlp {

enum class JobStatus {
    Queued,
    FetchingMetadata,
    Downloading,
    Completed,
    Failed,
    Cancelled
};

struct DownloadJob {
    QString id;
    QUrl url;
    QString targetDeckGroup;
    QString format;
    QString quality;
    QString downloadDir;
    JobStatus status = JobStatus::Queued;
    double progressPercent = 0.0;
    QString speed;
    QString eta;
    QString title;
    QString finalFilePath;
    QString errorMessage;
};

class YtDlpService : public QObject {
    Q_OBJECT

  public:
    explicit YtDlpService(
            UserSettingsPointer pConfig,
            TrackCollectionManager* pTrackCollectionManager,
            QObject* pParent = nullptr);
    ~YtDlpService() override;

    static QString findYtDlpBinary(const QString& customPath = QString());
    static bool isYtDlpAvailable(const QString& customPath = QString());
    static QString getYtDlpVersion(const QString& customPath = QString());

    /// Queue a URL for download and optional auto-load into a player deck
    QString startDownload(
            const QUrl& url,
            const QString& targetDeckGroup = QString(),
            const QString& format = QString(),
            const QString& quality = QString());

    /// Cancel a specific download job
    void cancelJob(const QString& jobId);

    /// Cancel all queued or running jobs
    void cancelAll();

    const QMap<QString, DownloadJob>& getJobs() const {
        return m_jobs;
    }

    YtDlpSettings& settings() {
        return m_settings;
    }

  signals:
    void jobStarted(const QString& jobId, const QUrl& url, const QString& targetDeckGroup);
    void jobProgress(const QString& jobId, double percent, const QString& speed, const QString& eta);
    void jobFinished(const QString& jobId, const QString& filePath, const QString& targetDeckGroup, TrackPointer pTrack);
    void jobFailed(const QString& jobId, const QString& errorMessage);
    void loadTrackToPlayer(TrackPointer pTrack, const QString& group, bool play = false);

  private slots:
    void slotProcessReadyReadStandardOutput();
    void slotProcessReadyReadStandardError();
    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotProcessErrorOccurred(QProcess::ProcessError error);

  private:
    void processNextJob();
    void startJob(const DownloadJob& job);
    void parseOutputLine(const QString& line);

    UserSettingsPointer m_pConfig;
    TrackCollectionManager* m_pTrackCollectionManager;
    YtDlpSettings m_settings;

    QMap<QString, DownloadJob> m_jobs;
    QQueue<QString> m_jobQueue;

    std::unique_ptr<QProcess> m_pCurrentProcess;
    QString m_currentJobId;
    QString m_lastStandardError;
};

} // namespace ytdlp
} // namespace mixxx
