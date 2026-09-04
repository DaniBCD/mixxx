// ==============================================================================
// DISCLAIMER: The following code and comments were autonomously generated
// by an AI Agent in accordance with the Mixxx Project AI Policy.
// ==============================================================================
#pragma once

#include "library/stems/stemenvironmentmanager.h"
#include "library/stems/stemsettings.h"
#include "preferences/dialog/dlgpreferencepage.h"
#include "preferences/dialog/ui_dlgprefstems.h"
#include "preferences/usersettings.h"

class DlgPrefStems : public DlgPreferencePage, public Ui::DlgPrefStems {
    Q_OBJECT

  public:
    DlgPrefStems(QWidget* pParent, UserSettingsPointer pConfig);
    ~DlgPrefStems() override = default;

  public slots:
    void slotUpdate() override;
    void slotApply() override;
    void slotResetToDefaults() override;

  private slots:
    void slotCheckStatus();
    void slotInstallEnv();
    void slotInstallProgress(int percent, const QString& status);
    void slotInstallFinished(bool success, const QString& message);
    void slotBrowseCustomDir();
    void slotBrowsePython();
    void slotBrowseFfmpeg();
    void slotRadioStorageToggled();

  private:
    UserSettingsPointer m_pConfig;
    mixxx::stems::StemSettings m_settings;
    mixxx::stems::StemEnvironmentManager m_envManager;
};
// ==============================================================================
// DISCLAIMER: End of autonomously generated code by AI Agent.
// ==============================================================================
