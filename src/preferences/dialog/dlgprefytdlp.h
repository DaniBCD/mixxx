#pragma once

#include "library/ytdlp/ytdlpservice.h"
#include "library/ytdlp/ytdlpsettings.h"
#include "preferences/dialog/dlgpreferencepage.h"
#include "preferences/dialog/ui_dlgprefytdlp.h"
#include "preferences/usersettings.h"

class DlgPrefYtDlp : public DlgPreferencePage, public Ui::DlgPrefYtDlp {
    Q_OBJECT

  public:
    DlgPrefYtDlp(QWidget* pParent, UserSettingsPointer pConfig);
    ~DlgPrefYtDlp() override = default;

  public slots:
    void slotUpdate() override;
    void slotApply() override;
    void slotResetToDefaults() override;

  private slots:
    void slotBrowseExecutable();
    void slotTestExecutable();
    void slotBrowseDownloadDir();

  private:
    UserSettingsPointer m_pConfig;
    mixxx::ytdlp::YtDlpSettings m_settings;
};
