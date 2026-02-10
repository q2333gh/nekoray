#pragma once

#include "main/NekoGui.hpp"
#include "db/traffic/TrafficData.hpp"
#include "fmt/AbstractBean.hpp"

namespace NekoGui_fmt {
    class SocksHttpBean;

    class ShadowSocksBean;

    class VMessBean;

    class TrojanVLESSBean;

    class NaiveBean;

    class QUICBean;

    class CustomBean;

    class ChainBean;
}; // namespace NekoGui_fmt

namespace NekoGui {
    class ProxyEntity : public JsonStore {
    public:
        QString type;

        int id = -1;
        int gid = 0;
        int latency = 0;
        std::shared_ptr<NekoGui_fmt::AbstractBean> bean;
        std::shared_ptr<NekoGui_traffic::TrafficData> traffic_data = std::make_shared<NekoGui_traffic::TrafficData>("");

        QString full_test_report;

        ProxyEntity(NekoGui_fmt::AbstractBean *bean, const QString &type_);

        [[nodiscard]] QString DisplayLatency() const;

        [[nodiscard]] QColor DisplayLatencyColor() const;

        // Typed accessors. Implemented out-of-line in `db/ProxyEntity.cpp` so we
        // can include the concrete bean types there without bloating headers.
        [[nodiscard]] NekoGui_fmt::ChainBean *ChainBean() const;
        [[nodiscard]] NekoGui_fmt::SocksHttpBean *SocksHTTPBean() const;
        [[nodiscard]] NekoGui_fmt::ShadowSocksBean *ShadowSocksBean() const;
        [[nodiscard]] NekoGui_fmt::VMessBean *VMessBean() const;
        [[nodiscard]] NekoGui_fmt::TrojanVLESSBean *TrojanVLESSBean() const;
        [[nodiscard]] NekoGui_fmt::NaiveBean *NaiveBean() const;
        [[nodiscard]] NekoGui_fmt::QUICBean *QUICBean() const;
        [[nodiscard]] NekoGui_fmt::CustomBean *CustomBean() const;
    };
} // namespace NekoGui
