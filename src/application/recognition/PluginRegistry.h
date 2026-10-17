#ifndef HAIKENANIME_PLUGINREGISTRY_H
#define HAIKENANIME_PLUGINREGISTRY_H

#include <QList>
#include <QString>

#include <memory>
#include <vector>

#include "IPlaybackPlugin.h"

/// Owns the internal playback plugins, in registration order.
class PluginRegistry final {
public:
    /// Rejects null plugins, blank ids and duplicate ids.
    bool registerPlugin(std::unique_ptr<IPlaybackPlugin> plugin);
    [[nodiscard]] QList<IPlaybackPlugin *> plugins() const;
    [[nodiscard]] IPlaybackPlugin *plugin(const QString &id) const;

private:
    std::vector<std::unique_ptr<IPlaybackPlugin>> plugins_;
};

#endif // HAIKENANIME_PLUGINREGISTRY_H
