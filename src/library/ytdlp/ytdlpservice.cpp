#include "library/ytdlp/ytdlpservice.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

#include "library/trackcollectionmanager.h"
#include "moc_ytdlpservice.cpp"
#include "track/trackref.h"
#include "util/logger.h"

namespace mixxx {
namespace ytdlp {

namespace {
const Logger kLogger("YtDlpService");
} // anonymous namespace

YtDlpService::YtDlpService(
        UserSettingsPointer pConfig,
        TrackCollectionManager* pTrackCollectionManager,
        QObject* pParent)
        : QObject(pParent),
          m_pConfig(pConfig),
          m_pTrackCollectionManager(pTrackCollectionManager),
          m_settings(pConfig) {
}

YtDlpService::~YtDlpService() {
    cancelAll();
}

QString YtDlpService::findYtDlpBinary(const QString& customPath) {
    if (!customPath.trimmed().isEmpty()) {
        QFileInfo customInfo(customPath.trimmed());
        if (customInfo.exists() && customInfo.isExecutable()) {
            return customInfo.canonicalFilePath();
        }
    }

    // Try finding via system PATH
    QString inPath = QStandardPaths::findExecutable(QStringLiteral("yt-dlp"));
    if (!inPath.isEmpty()) {
        return inPath;
    }
#ifdef Q_OS_WIN
    inPath = QStandardPaths::findExecutable(QStringLiteral("yt-dlp.exe"));
    if (!inPath.isEmpty()) {
        return inPath;
    }
    // Check common Windows locations
    QStringList winPaths = {
            QDir::homePath() + QStringLiteral("/AppData/Local/Programs/yt-dlp/yt-dlp.exe"),
            QDir::homePath() + QStringLiteral("/AppData/Local/yt-dlp/yt-dlp.exe"),
            QDir::homePath() + QStringLiteral("/AppData/Roaming/yt-dlp/yt-dlp.exe"),
            QStringLiteral("C:/yt-dlp/yt-dlp.exe"),
            QStringLiteral("C:/ProgramData/chocolatey/bin/yt-dlp.exe"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/yt-dlp.exe"),
    };
    for (const auto& p : winPaths) {
        if (QFileInfo::exists(p)) {
            return p;
        }
    }
#else
    // Check common Unix locations
    QStringList unixPaths = {
            QStringLiteral("/usr/local/bin/yt-dlp"),
            QStringLiteral("/opt/homebrew/bin/yt-dlp"),
            QStringLiteral("/usr/bin/yt-dlp"),
            QDir::homePath() + QStringLiteral("/.local/bin/yt-dlp"),
            QDir::homePath() + QStringLiteral("/bin/yt-dlp"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/yt-dlp"),
    };
    for (const auto& p : unixPaths) {
        if (QFileInfo::exists(p)) {
            return p;
        }
    }
#endif
    return QString();
}

bool YtDlpService::isYtDlpAvailable(const QString& customPath) {
    return !findYtDlpBinary(customPath).isEmpty();
}

QString YtDlpService::getYtDlpVersion(const QString& customPath) {
    QString bin = findYtDlpBinary(customPath);
    if (bin.isEmpty()) {
        return QString();
    }
    QProcess proc;
    proc.start(bin, {QStringLiteral("--version")});
    if (proc.waitForFinished(5000)) {
        return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    return QString();
}

QString YtDlpService::startDownload(
        const QUrl& url,
        const QString& targetDeckGroup,
        const QString& format,
        const QString& quality) {
    QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    DownloadJob job;
    job.id = jobId;
    job.url = url;
    job.targetDeckGroup = targetDeckGroup;
    job.format = format.isEmpty() ? m_settings.getAudioFormat() : format;
    job.quality = quality.isEmpty() ? m_settings.getAudioQuality() : quality;
    job.downloadDir = m_settings.getDownloadDirectory();
    job.status = JobStatus::Queued;

    m_jobs[jobId] = job;
    m_jobQueue.enqueue(jobId);

    kLogger.info() << "Enqueued yt-dlp download job:" << jobId << "URL:" << url.toString()
                   << "Target Deck:" << targetDeckGroup;

    if (!m_pCurrentProcess) {
        processNextJob();
    }

    return jobId;
}

void YtDlpService::cancelJob(const QString& jobId) {
    if (m_currentJobId == jobId && m_pCurrentProcess) {
        m_pCurrentProcess->kill();
        if (m_jobs.contains(jobId)) {
            m_jobs[jobId].status = JobStatus::Cancelled;
        }
        return;
    }

    m_jobQueue.removeAll(jobId);
    if (m_jobs.contains(jobId)) {
        m_jobs[jobId].status = JobStatus::Cancelled;
    }
}

void YtDlpService::cancelAll() {
    m_jobQueue.clear();
    if (m_pCurrentProcess) {
        m_pCurrentProcess->kill();
    }
    for (auto& job : m_jobs) {
        if (job.status == JobStatus::Queued || job.status == JobStatus::Downloading) {
            job.status = JobStatus::Cancelled;
        }
    }
}

void YtDlpService::processNextJob() {
    if (m_jobQueue.isEmpty()) {
        m_currentJobId.clear();
        m_pCurrentProcess.reset();
        return;
    }

    m_currentJobId = m_jobQueue.dequeue();
    if (!m_jobs.contains(m_currentJobId)) {
        processNextJob();
        return;
    }

    startJob(m_jobs[m_currentJobId]);
}

void YtDlpService::startJob(const DownloadJob& job) {
    QString bin = findYtDlpBinary(m_settings.getExecutablePath());
    if (bin.isEmpty()) {
        QString errorMsg = tr("yt-dlp executable not found. Please verify preferences.");
        kLogger.warning() << errorMsg;
        m_jobs[job.id].status = JobStatus::Failed;
        m_jobs[job.id].errorMessage = errorMsg;
        emit jobFailed(job.id, errorMsg);
        processNextJob();
        return;
    }

    // Ensure output directory exists
    QDir dir(job.downloadDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    m_jobs[job.id].status = JobStatus::Downloading;
    m_lastStandardError.clear();

    QStringList args;
    args << QStringLiteral("-x")
         << QStringLiteral("--audio-format") << job.format
         << QStringLiteral("--audio-quality") << job.quality
         << QStringLiteral("--no-playlist");

    if (m_settings.getEmbedMetadata()) {
        args << QStringLiteral("--embed-metadata");
    }
    if (m_settings.getEmbedThumbnail()) {
        args << QStringLiteral("--embed-thumbnail");
    }

    QString cookiesBrowser = m_settings.getCookiesBrowser();
    if (!cookiesBrowser.trimmed().isEmpty()) {
        args << QStringLiteral("--cookies-from-browser") << cookiesBrowser.trimmed();
    }

    QString outputTemplate = dir.filePath(QStringLiteral("%(title)s [%(id)s].%(ext)s"));
    args << QStringLiteral("-o") << outputTemplate
         << QStringLiteral("--newline")
         << QStringLiteral("--print") << QStringLiteral("after_move:filepath:%(filepath)s")
         << QStringLiteral("--progress-template") << QStringLiteral("PROG:%(progress._percent_str)s|%(progress._speed_str)s|%(progress._eta_str)s")
         << job.url.toString();

    m_pCurrentProcess = std::make_unique<QProcess>(this);
    connect(m_pCurrentProcess.get(), &QProcess::readyReadStandardOutput,
            this, &YtDlpService::slotProcessReadyReadStandardOutput);
    connect(m_pCurrentProcess.get(), &QProcess::readyReadStandardError,
            this, &YtDlpService::slotProcessReadyReadStandardError);
    connect(m_pCurrentProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YtDlpService::slotProcessFinished);
    connect(m_pCurrentProcess.get(), &QProcess::errorOccurred,
            this, &YtDlpService::slotProcessErrorOccurred);

    kLogger.info() << "Starting yt-dlp process:" << bin << args.join(" ");
    emit jobStarted(job.id, job.url, job.targetDeckGroup);
    m_pCurrentProcess->start(bin, args);
}

void YtDlpService::slotProcessReadyReadStandardOutput() {
    if (!m_pCurrentProcess) {
        return;
    }
    while (m_pCurrentProcess->canReadLine()) {
        QString line = QString::fromUtf8(m_pCurrentProcess->readLine()).trimmed();
        if (!line.isEmpty()) {
            parseOutputLine(line);
        }
    }
}

void YtDlpService::parseOutputLine(const QString& line) {
    if (m_currentJobId.isEmpty() || !m_jobs.contains(m_currentJobId)) {
        return;
    }
    DownloadJob& job = m_jobs[m_currentJobId];

    if (line.startsWith(QStringLiteral("PROG:"))) {
        QString content = line.mid(5);
        QStringList parts = content.split(QLatin1Char('|'));
        if (!parts.isEmpty()) {
            QString percentStr = parts[0].trimmed().remove(QLatin1Char('%'));
            bool ok = false;
            double percent = percentStr.toDouble(&ok);
            if (ok) {
                job.progressPercent = percent;
            }
        }
        if (parts.size() > 1) {
            job.speed = parts[1].trimmed();
        }
        if (parts.size() > 2) {
            job.eta = parts[2].trimmed();
        }
        emit jobProgress(job.id, job.progressPercent, job.speed, job.eta);
    } else if (line.startsWith(QStringLiteral("filepath:"))) {
        job.finalFilePath = line.mid(9).trimmed();
        kLogger.info() << "Captured download final file path:" << job.finalFilePath;
    } else if (line.startsWith(QStringLiteral("[download] Destination: "))) {
        job.finalFilePath = line.mid(24).trimmed();
    }
}

void YtDlpService::slotProcessReadyReadStandardError() {
    if (!m_pCurrentProcess) {
        return;
    }
    QString err = QString::fromUtf8(m_pCurrentProcess->readAllStandardError());
    m_lastStandardError.append(err);
    kLogger.debug() << "yt-dlp stderr:" << err;
}

void YtDlpService::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_currentJobId.isEmpty() || !m_jobs.contains(m_currentJobId)) {
        processNextJob();
        return;
    }

    DownloadJob& job = m_jobs[m_currentJobId];

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        job.status = JobStatus::Completed;
        job.progressPercent = 100.0;
        kLogger.info() << "yt-dlp job completed successfully:" << job.id << "Path:" << job.finalFilePath;

        TrackPointer pTrack;
        if (!job.finalFilePath.isEmpty() && QFileInfo::exists(job.finalFilePath)) {
            if (m_pTrackCollectionManager) {
                pTrack = m_pTrackCollectionManager->getOrAddTrack(
                        TrackRef::fromFilePath(job.finalFilePath));
            }
        }

        if (pTrack && !job.targetDeckGroup.isEmpty()) {
            kLogger.info() << "Auto-loading track into deck group:" << job.targetDeckGroup;
            emit loadTrackToPlayer(pTrack, job.targetDeckGroup, false);
        }

        emit jobFinished(job.id, job.finalFilePath, job.targetDeckGroup, pTrack);
    } else {
        job.status = JobStatus::Failed;
        QString errorMsg = m_lastStandardError.trimmed();
        if (errorMsg.isEmpty()) {
            errorMsg = tr("yt-dlp process exited with code %1").arg(exitCode);
        }
        job.errorMessage = errorMsg;
        kLogger.warning() << "yt-dlp job failed:" << job.id << errorMsg;
        emit jobFailed(job.id, errorMsg);
    }

    processNextJob();
}

void YtDlpService::slotProcessErrorOccurred(QProcess::ProcessError error) {
    if (m_currentJobId.isEmpty() || !m_jobs.contains(m_currentJobId)) {
        return;
    }
    DownloadJob& job = m_jobs[m_currentJobId];
    job.status = JobStatus::Failed;
    QString errorMsg = tr("Process error (%1): %2").arg(error).arg(m_lastStandardError);
    job.errorMessage = errorMsg;
    kLogger.warning() << "yt-dlp process error:" << job.id << errorMsg;
    emit jobFailed(job.id, errorMsg);
}

} // namespace ytdlp
} // namespace mixxx
