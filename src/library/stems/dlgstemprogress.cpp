// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#include "library/stems/dlgstemprogress.h"

#include <QFileInfo>

#include "moc_dlgstemprogress.cpp"
#include "track/track.h"

namespace mixxx {
namespace stems {

DlgStemProgress::DlgStemProgress(
        StemSeparationService* pService,
        const QString& jobId,
        QWidget* pParent)
        : QDialog(pParent),
          m_pService(pService),
          m_jobId(jobId) {
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    connect(btnCancel, &QPushButton::clicked, this, &DlgStemProgress::slotCancelClicked);
    connect(btnClose, &QPushButton::clicked, this, &DlgStemProgress::slotCloseClicked);

    if (m_pService) {
        connect(m_pService, &StemSeparationService::jobProgress,
                this, &DlgStemProgress::slotJobProgress);
        connect(m_pService, &StemSeparationService::jobFinished,
                this, &DlgStemProgress::slotJobFinished);
        connect(m_pService, &StemSeparationService::jobFailed,
                this, &DlgStemProgress::slotJobFailed);

        const auto& jobs = m_pService->getJobs();
        if (jobs.contains(jobId)) {
            const StemJob& job = jobs[jobId];
            QString title = job.pSourceTrack ? job.pSourceTrack->getTitleInfo() : QFileInfo(job.sourceFilePath).fileName();
            labelTrackTitle->setText(tr("Pista: %1").arg(title));
            labelMode->setText(job.mode == SeparationMode::TwoStems_Vocals_Instrumental
                    ? tr("Modo: Separación de Voz e Instrumental (2 stems)")
                    : tr("Modo: Separación completa de 4 stems (Voz, Batería, Bajo, Otros)"));
            progressBar->setValue(job.progressPercent);
            labelStatus->setText(job.currentStepMessage.isEmpty() ? tr("Preparando separación...") : job.currentStepMessage);
        }
    }
}

void DlgStemProgress::slotCancelClicked() {
    if (m_pService && !m_jobId.isEmpty()) {
        m_pService->cancelJob(m_jobId);
    }
    labelStatus->setText(tr("Cancelado por el usuario."));
    btnCancel->setEnabled(false);
    btnClose->setEnabled(true);
}

void DlgStemProgress::slotCloseClicked() {
    accept();
}

void DlgStemProgress::slotJobProgress(const QString& jobId, int percent, const QString& stepMessage) {
    if (jobId != m_jobId) {
        return;
    }
    progressBar->setValue(percent);
    labelStatus->setText(stepMessage);
}

void DlgStemProgress::slotJobFinished(const QString& jobId, const QString& stemFilePath, const QString& targetDeckGroup, TrackPointer pStemTrack) {
    Q_UNUSED(targetDeckGroup);
    Q_UNUSED(pStemTrack);
    if (jobId != m_jobId) {
        return;
    }
    progressBar->setValue(100);
    labelStatus->setText(tr("¡Separación completada con éxito!\nGuardado en: %1").arg(stemFilePath));
    btnCancel->setEnabled(false);
    btnClose->setEnabled(true);
}

void DlgStemProgress::slotJobFailed(const QString& jobId, const QString& errorMessage) {
    if (jobId != m_jobId) {
        return;
    }
    labelStatus->setText(tr("Error en la separación:\n%1").arg(errorMessage));
    btnCancel->setEnabled(false);
    btnClose->setEnabled(true);
}

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
