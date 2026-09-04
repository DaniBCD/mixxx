// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#pragma once

#include <QDialog>

#include "library/stems/stemseparationservice.h"
#include "library/stems/ui_dlgstemprogress.h"

namespace mixxx {
namespace stems {

class DlgStemProgress : public QDialog, public Ui::DlgStemProgress {
    Q_OBJECT

  public:
    DlgStemProgress(
            StemSeparationService* pService,
            const QString& jobId,
            QWidget* pParent = nullptr);
    ~DlgStemProgress() override = default;

  private slots:
    void slotCancelClicked();
    void slotCloseClicked();
    void slotJobProgress(const QString& jobId, int percent, const QString& stepMessage);
    void slotJobFinished(const QString& jobId, const QString& stemFilePath, const QString& targetDeckGroup, TrackPointer pStemTrack);
    void slotJobFailed(const QString& jobId, const QString& errorMessage);

  private:
    StemSeparationService* m_pService;
    QString m_jobId;
};

} // namespace stems
} // namespace mixxx
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
