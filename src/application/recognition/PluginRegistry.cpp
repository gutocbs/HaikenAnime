#include "PluginRegistry.h"

#include <algorithm>

bool PluginRegistry::registerPlugin(std::unique_ptr<IPlaybackPlugin> plugin) {
    if (!plugin || plugin->id().trimmed().isEmpty() || this->plugin(plugin->id()) != nullptr) return false;
    plugins_.push_back(std::move(plugin));
    return true;
}

QList<IPlaybackPlugin *> PluginRegistry::plugins() const {
    QList<IPlaybackPlugin *> result;
    result.reserve(static_cast<qsizetype>(plugins_.size()));
    for (const auto &plugin : plugins_) result.append(plugin.get());
    return result;
}

IPlaybackPlugin *PluginRegistry::plugin(const QString &id) const {
    const auto found = std::find_if(plugins_.cbegin(), plugins_.cend(),
                                    [&id](const auto &candidate) { return candidate->id() == id; });
    return found == plugins_.cend() ? nullptr : found->get();
}
