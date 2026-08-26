#pragma once

#include <QList>
#include <QObject>
#include <QUrl>
#include <QVariant>

#include "library/libraryfeature.h"
#include "library/treeitemmodel.h"
#include "library/ytdlp/ytdlpservice.h"
#include "preferences/usersettings.h"
#include "util/parented_ptr.h"

namespace mixxx {
namespace ytdlp {

class DlgDownloadUrl;

class YtDlpFeature : public LibraryFeature {
    Q_OBJECT

  public:
    YtDlpFeature(
            Library* pLibrary,
            UserSettingsPointer pConfig,
            YtDlpService* pService);
    ~YtDlpFeature() override = default;

    QVariant title() override {
        return m_title;
    }

    bool dropAccept(const QList<QUrl>& urls, QObject* pSource) override;
    bool dragMoveAccept(const QList<QUrl>& urls) override;
    void bindLibraryWidget(WLibrary* libraryWidget, KeyboardEventFilter* keyboard) override;
    TreeItemModel* sidebarModel() const override;

  public slots:
    void activate() override;
    void slotOpenDownloadDialog(const QUrl& url = QUrl(), const QString& targetDeck = QString());

  private slots:
    void slotJobStarted(const QString& jobId, const QUrl& url, const QString& targetDeckGroup);
    void slotJobProgress(const QString& jobId, double percent, const QString& speed, const QString& eta);
    void slotJobFinished(const QString& jobId, const QString& filePath, const QString& targetDeckGroup, TrackPointer pTrack);
    void slotJobFailed(const QString& jobId, const QString& errorMessage);

  private:
    QString m_baseTitle;
    QString m_title;
    YtDlpService* m_pService;
    parented_ptr<TreeItemModel> m_pSidebarModel;
};

} // namespace ytdlp
} // namespace mixxx
