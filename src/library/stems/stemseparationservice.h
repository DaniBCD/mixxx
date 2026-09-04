// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <memory>

#include "library/stems/stemenvironmentmanager.h"
#include "library/stems/stemsettings.h"
#include "library/trackcollectionmanager.h"
#include "track/track_decl.h"

namespace mixxx {
namespace stems {

enum class SeparationMode {
    TwoStems_Vocals_Instrumental,
    FourStems_Complete
};

enum class JobStatus {
    Queued,
    Separating,
    Encoding,
    Completed,
    Failed,
    Cancelled
};

struct StemJob {
    QString id;
    TrackPointer pSourceTrack;
    QString sourceFilePath;
    QString outputStemPath;
    SeparationMode mode = SeparationMode::TwoStems_Vocals_Instrumental;
    QString targetDeckGroup;
    JobStatus status = JobStatus::Queued;
    int progressPercent = 0;
    QString currentStepMessage;
    QString errorMessage;
};

class StemSeparationService : public QObject {
    Q_OBJECT

  public:
    explicit StemSeparationService(
            UserSettingsPointer pConfig,
            TrackCollectionManager* pTrackCollectionManager,
            QObject* pParent = nullptr);
    ~StemSeparationService() override;

    /// Queues a track for AI vocal/instrumental separation
    QString startSeparation(
            TrackPointer pTrack,
            SeparationMode mode = SeparationMode::TwoStems_Vocals_Instrumental,
            const QString& targetDeckGroup = QString());

    /// Cancels a specific separation job
    void cancelJob(const QString& jobId);

    /// Cancels all queued and active jobs
    void cancelAll();

    const QMap<QString, StemJob>& getJobs() const {
        return m_jobs;
    }

    StemSettings& settings() {
        return m_settings;
    }

    StemEnvironmentManager* environmentManager() {
        return &m_envManager;
    }

  signals:
    void jobStarted(const QString& jobId, TrackPointer pSourceTrack, const QString& targetDeckGroup);
    void jobProgress(const QString& jobId, int percent, const QString& stepMessage);
    void jobFinished(const QString& jobId, const QString& stemFilePath, const QString& targetDeckGroup, TrackPointer pStemTrack);
    void jobFailed(const QString& jobId, const QString& errorMessage);
    void loadTrackToPlayer(TrackPointer pTrack, const QString& group, bool play = false);

  private slots:
    void slotProcessReadyReadStandardOutput();
    void slotProcessReadyReadStandardError();
    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotProcessErrorOccurred(QProcess::ProcessError error);

  private:
    void processNextJob();
    void startJob(const StemJob& job);
    void parseOutputLine(const QString& line);
    QString resolveHelperScriptPath() const;
    QString determineOutputPath(const QString& sourcePath) const;

    UserSettingsPointer m_pConfig;
    TrackCollectionManager* m_pTrackCollectionManager;
    StemSettings m_settings;
    StemEnvironmentManager m_envManager;

    QMap<QString, StemJob> m_jobs;
    QQueue<QString> m_jobQueue;

    std::unique_ptr<QProcess> m_pCurrentProcess;
    QString m_currentJobId;
    QString m_lastStandardError;
};

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
