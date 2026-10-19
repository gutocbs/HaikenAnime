#ifndef HAIKENANIME_PRESENTATIONCOMPOSITION_H
#define HAIKENANIME_PRESENTATIONCOMPOSITION_H

#include <memory>

#include "ApplicationComposition.h"
#include "../application/anilist/RemoveAniListAccountUseCase.h"
#include "../application/media/SeasonalPersonalListService.h"
#include "../presentation/home/HomeScreenController.h"
#include "../presentation/recognition/EpisodeRecognitionController.h"
#include "../presentation/recognition/NowWatchingController.h"
#include "../presentation/seasonal/SeasonalCatalogController.h"
#include "../presentation/settings/PluginSettingsController.h"
#include "../presentation/settings/SettingsController.h"
#include "../presentation/sync/SynchronizationStatusController.h"

class AniListOAuthCallbackReceiver;
class QQmlContext;

/// Controllers and screen services exposed to QML. Members are created in declaration order and destroyed in
/// reverse, so the recognition screens go first and the Home last, as the services they observe require.
struct PresentationContext final {
    std::unique_ptr<HomeScreenController> home;
    std::unique_ptr<SeasonalPersonalListService> personalLists;
    std::unique_ptr<SeasonalCatalogController> seasonalCatalog;
    std::unique_ptr<SettingsController> settings;
    std::unique_ptr<SynchronizationStatusController> synchronizationStatus;
    std::unique_ptr<RemoveAniListAccountUseCase> removeAniListAccount;
    /// Null when episode recognition could not be composed.
    std::unique_ptr<PluginSettingsController> pluginSettings;
    std::unique_ptr<EpisodeRecognitionController> recognition;
    std::unique_ptr<NowWatchingController> nowWatching;
};

/// Creates the controllers and connects them to the application services (reloads after sync and edits,
/// local recognition requests, AniList account flows, preferences applied at runtime). context and
/// oauthCallbackReceiver must outlive the result; lifetime owns the connections that have no controller.
[[nodiscard]] std::unique_ptr<PresentationContext> CreatePresentationContext(
    ApplicationContext &context, AniListOAuthCallbackReceiver &oauthCallbackReceiver, QObject &lifetime);
/// Registers the controllers under the context property names the QML uses.
void ExposeToQml(PresentationContext &presentation, QQmlContext &qml);

#endif // HAIKENANIME_PRESENTATIONCOMPOSITION_H
