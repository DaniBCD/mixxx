// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#include "library/stems/stemseparationservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

#include "library/dao/trackdao.h"
#include "library/trackcollection.h"
#include "moc_stemseparationservice.cpp"
#include "track/trackref.h"
#include "util/logger.h"

namespace mixxx {
namespace stems {

namespace {
const Logger kLogger("StemSeparationService");
} // anonymous namespace

StemSeparationService::StemSeparationService(
        UserSettingsPointer pConfig,
        TrackCollectionManager* pTrackCollectionManager,
        QObject* pParent)
        : QObject(pParent),
          m_pConfig(pConfig),
          m_pTrackCollectionManager(pTrackCollectionManager),
          m_settings(pConfig),
          m_envManager(&m_settings, this) {
}

StemSeparationService::~StemSeparationService() {
    cancelAll();
}

QString StemSeparationService::resolveHelperScriptPath() const {
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
            appDir + QStringLiteral("/tools/stems/mixxx_stem_creator.py"),
            appDir + QStringLiteral("/../tools/stems/mixxx_stem_creator.py"),
            appDir + QStringLiteral("/../../tools/stems/mixxx_stem_creator.py"),
            QDir::currentPath() + QStringLiteral("/tools/stems/mixxx_stem_creator.py"),
    };
    for (const auto& path : candidates) {
        if (QFileInfo::exists(path)) {
            return QFileInfo(path).canonicalFilePath();
        }
    }
    return QString();
}

QString StemSeparationService::determineOutputPath(const QString& sourcePath) const {
    QFileInfo srcInfo(sourcePath);
    QString baseName = srcInfo.completeBaseName();
    QString stemFileName = baseName + QStringLiteral(".stem.mp4");

    if (m_settings.getSaveInOriginalDirectory()) {
        return srcInfo.dir().filePath(stemFileName);
    }

    QString customDir = m_settings.getCustomSaveDirectory();
    QDir().mkpath(customDir);
    return QDir(customDir).filePath(stemFileName);
}

QString StemSeparationService::startSeparation(
        TrackPointer pTrack,
        SeparationMode mode,
        const QString& targetDeckGroup) {
    if (!pTrack) {
        kLogger.warning() << "Cannot start separation on null track";
        return QString();
    }

    QString sourcePath = pTrack->getLocation();
    if (!QFileInfo::exists(sourcePath)) {
        kLogger.warning() << "Source track file does not exist:" << sourcePath;
        return QString();
    }

    EnvironmentStatus envStatus = m_envManager.checkStatus();
    if (!envStatus.isReady()) {
        QString err = tr("El entorno de IA no está listo (Demucs o FFmpeg no encontrados). Configúralo en Preferencias -> Stems.");
        emit jobFailed(QString(), err);
        return QString();
    }

    StemJob job;
    job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    job.pSourceTrack = pTrack;
    job.sourceFilePath = sourcePath;
    job.outputStemPath = determineOutputPath(sourcePath);
    job.mode = mode;
    job.targetDeckGroup = targetDeckGroup;
    job.status = JobStatus::Queued;
    job.progressPercent = 0;
    job.currentStepMessage = tr("En cola de espera...");

    m_jobs.insert(job.id, job);
    m_jobQueue.enqueue(job.id);

    kLogger.info() << "Queued stem separation job" << job.id
                   << "for" << sourcePath
                   << "mode:" << (mode == SeparationMode::TwoStems_Vocals_Instrumental ? "2stems" : "4stems");

    if (!m_pCurrentProcess) {
        processNextJob();
    }

    return job.id;
}

void StemSeparationService::cancelJob(const QString& jobId) {
    if (m_currentJobId == jobId && m_pCurrentProcess) {
        kLogger.info() << "Cancelling running stem separation job" << jobId;
        m_pCurrentProcess->kill();
        m_pCurrentProcess->waitForFinished(2000);
        if (m_jobs.contains(jobId)) {
            m_jobs[jobId].status = JobStatus::Cancelled;
        }
        m_currentJobId.clear();
        m_pCurrentProcess.reset();
        processNextJob();
        return;
    }

    m_jobQueue.removeAll(jobId);
    if (m_jobs.contains(jobId)) {
        m_jobs[jobId].status = JobStatus::Cancelled;
    }
}

void StemSeparationService::cancelAll() {
    m_jobQueue.clear();
    if (m_pCurrentProcess && m_pCurrentProcess->state() != QProcess::NotRunning) {
        m_pCurrentProcess->kill();
        m_pCurrentProcess->waitForFinished(2000);
    }
    m_currentJobId.clear();
    m_pCurrentProcess.reset();
}

void StemSeparationService::processNextJob() {
    if (m_jobQueue.isEmpty()) {
        m_pCurrentProcess.reset();
        m_currentJobId.clear();
        return;
    }

    m_currentJobId = m_jobQueue.dequeue();
    if (!m_jobs.contains(m_currentJobId)) {
        processNextJob();
        return;
    }

    startJob(m_jobs[m_currentJobId]);
}

void StemSeparationService::startJob(const StemJob& job) {
    QString scriptPath = resolveHelperScriptPath();
    if (scriptPath.isEmpty()) {
        QString err = tr("No se encontró el script auxiliar tools/stems/mixxx_stem_creator.py.");
        kLogger.warning() << err;
        m_jobs[job.id].status = JobStatus::Failed;
        m_jobs[job.id].errorMessage = err;
        emit jobFailed(job.id, err);
        processNextJob();
        return;
    }

    EnvironmentStatus env = m_envManager.checkStatus();
    QString pythonBin = env.pythonPath;
    if (pythonBin.isEmpty()) {
        QString err = tr("Ejecutable de Python no configurado.");
        m_jobs[job.id].status = JobStatus::Failed;
        m_jobs[job.id].errorMessage = err;
        emit jobFailed(job.id, err);
        processNextJob();
        return;
    }

    QStringList args = {
            scriptPath,
            QStringLiteral("--input"), job.sourceFilePath,
            QStringLiteral("--output"), job.outputStemPath,
            QStringLiteral("--mode"), (job.mode == SeparationMode::TwoStems_Vocals_Instrumental ? QStringLiteral("2stems") : QStringLiteral("4stems")),
            QStringLiteral("--model"), m_settings.getDemucsModel(),
            QStringLiteral("--device"), m_settings.getDemucsDevice(),
    };

    if (!env.ffmpegPath.isEmpty()) {
        args << QStringLiteral("--ffmpeg") << env.ffmpegPath;
    }

    m_pCurrentProcess = std::make_unique<QProcess>(this);
    connect(m_pCurrentProcess.get(), &QProcess::readyReadStandardOutput,
            this, &StemSeparationService::slotProcessReadyReadStandardOutput);
    connect(m_pCurrentProcess.get(), &QProcess::readyReadStandardError,
            this, &StemSeparationService::slotProcessReadyReadStandardError);
    connect(m_pCurrentProcess.get(), &QProcess::finished,
            this, &StemSeparationService::slotProcessFinished);
    connect(m_pCurrentProcess.get(), &QProcess::errorOccurred,
            this, &StemSeparationService::slotProcessErrorOccurred);

    m_jobs[job.id].status = JobStatus::Separating;
    m_lastStandardError.clear();

    kLogger.info() << "Starting separation process:" << pythonBin << args;
    emit jobStarted(job.id, job.pSourceTrack, job.targetDeckGroup);

    QProcessEnvironment procEnv = QProcessEnvironment::systemEnvironment();
    QStringList extraSitePaths = {
            QDir::homePath() + QStringLiteral("/AppData/Roaming/Python/Python311/site-packages"),
            QDir::homePath() + QStringLiteral("/AppData/Roaming/Python/Python312/site-packages"),
            QDir::homePath() + QStringLiteral("/AppData/Roaming/Python/Python310/site-packages"),
            QDir::homePath() + QStringLiteral("/AppData/Roaming/Python/Python313/site-packages"),
    };
    QString pyPath = procEnv.value(QStringLiteral("PYTHONPATH"));
    for (const auto& p : extraSitePaths) {
        if (QDir(p).exists()) {
            pyPath = p + (pyPath.isEmpty() ? QString() : QStringLiteral(";") + pyPath);
        }
    }
    procEnv.insert(QStringLiteral("PYTHONPATH"), pyPath);
    m_pCurrentProcess->setProcessEnvironment(procEnv);

    m_pCurrentProcess->start(pythonBin, args);
}

void StemSeparationService::slotProcessReadyReadStandardOutput() {
    if (!m_pCurrentProcess) {
        return;
    }
    while (m_pCurrentProcess->canReadLine()) {
        QString line = QString::fromUtf8(m_pCurrentProcess->readLine()).trimmed();
        parseOutputLine(line);
    }
}

void StemSeparationService::slotProcessReadyReadStandardError() {
    if (!m_pCurrentProcess) {
        return;
    }
    QString err = QString::fromUtf8(m_pCurrentProcess->readAllStandardError());
    m_lastStandardError += err;
    kLogger.debug() << "[StemCreator STDERR]" << err.trimmed();
}

void StemSeparationService::parseOutputLine(const QString& line) {
    if (line.startsWith(QLatin1String("PROGRESS:"))) {
        QStringList parts = line.split(QLatin1Char(':'));
        if (parts.size() >= 3) {
            bool ok = false;
            int pct = parts[1].toInt(&ok);
            if (ok && m_jobs.contains(m_currentJobId)) {
                QString msg = parts.mid(2).join(QLatin1Char(':'));
                m_jobs[m_currentJobId].progressPercent = pct;
                m_jobs[m_currentJobId].currentStepMessage = msg;
                emit jobProgress(m_currentJobId, pct, msg);
            }
        }
    } else {
        kLogger.debug() << "[StemCreator STDOUT]" << line;
    }
}

void StemSeparationService::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    QString finishedJobId = m_currentJobId;
    m_pCurrentProcess.reset();
    m_currentJobId.clear();

    if (!m_jobs.contains(finishedJobId)) {
        processNextJob();
        return;
    }

    StemJob& job = m_jobs[finishedJobId];

    if (exitCode == 0 && QFileInfo::exists(job.outputStemPath)) {
        job.status = JobStatus::Completed;
        job.progressPercent = 100;
        job.currentStepMessage = tr("Separación completada con éxito.");

        kLogger.info() << "Stem separation finished successfully:" << job.outputStemPath;

        TrackPointer pStemTrack;
        if (m_pTrackCollectionManager) {
            pStemTrack = m_pTrackCollectionManager->getOrAddTrack(
                    TrackRef::fromFilePath(job.outputStemPath));
        }

        emit jobFinished(finishedJobId, job.outputStemPath, job.targetDeckGroup, pStemTrack);

        if (pStemTrack && !job.targetDeckGroup.isEmpty() && m_settings.getAutoLoadToDeck()) {
            emit loadTrackToPlayer(pStemTrack, job.targetDeckGroup, false);
        }
    } else {
        job.status = JobStatus::Failed;
        QString err = m_lastStandardError.trimmed();
        if (err.isEmpty()) {
            err = tr("El proceso de separación falló con código de salida %1").arg(exitCode);
        }
        job.errorMessage = err;
        kLogger.warning() << "Stem separation failed for job" << finishedJobId << ":" << err;
        emit jobFailed(finishedJobId, err);
    }

    processNextJob();
}

void StemSeparationService::slotProcessErrorOccurred(QProcess::ProcessError error) {
    Q_UNUSED(error);
    if (!m_currentJobId.isEmpty() && m_jobs.contains(m_currentJobId)) {
        QString err = tr("Error al iniciar el proceso de separación de stems.");
        m_jobs[m_currentJobId].status = JobStatus::Failed;
        m_jobs[m_currentJobId].errorMessage = err;
        emit jobFailed(m_currentJobId, err);
    }
}

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
