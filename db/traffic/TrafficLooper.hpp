#pragma once

#include <atomic>
#include <memory>
#include <QString>
#include <QList>
#include <QMutex>

#include "TrafficData.hpp"

namespace NekoGui_traffic {
    class TrafficLooper {
    public:
        std::atomic<bool> loop_enabled{false};
        std::atomic<bool> looping{false};
        QMutex loop_mutex;

        QList<std::shared_ptr<TrafficData>> items;
        TrafficData *proxy = nullptr; // Non-owning pointer into items list

        void UpdateAll();

        void Loop();

    private:
        std::unique_ptr<TrafficData> bypass = std::make_unique<TrafficData>("bypass");

        [[nodiscard]] static std::unique_ptr<TrafficData> update_stats(TrafficData *item);

        [[nodiscard]] static QJsonArray get_connection_list();
    };

    extern TrafficLooper *trafficLooper;
} // namespace NekoGui_traffic
