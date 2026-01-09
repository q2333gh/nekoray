#pragma once

#include "db/Database.hpp"

namespace NekoGui_sub {
    class RawUpdater {
    public:
        void updateClash(const QString &str);

        void update(const QString &str);

        int gid_add_to = -1; // 导入到指定组 -1 为当前选中组

        QList<std::shared_ptr<NekoGui::ProxyEntity>> updated_order; // 新增的配置，按照导入时处理的先后排序

    private:
        // Protocol parsing helpers
        std::shared_ptr<NekoGui::ProxyEntity> parseNekorayLink(const QString &str, bool &needFix);
        std::shared_ptr<NekoGui::ProxyEntity> parseSocksLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseHttpLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseShadowsocksLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseVMessLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseVLESSLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseTrojanLink(const QString &str);
        std::shared_ptr<NekoGui::ProxyEntity> parseNaiveLink(const QString &str, bool &needFix);
        std::shared_ptr<NekoGui::ProxyEntity> parseHysteria2Link(const QString &str, bool &needFix);
        std::shared_ptr<NekoGui::ProxyEntity> parseTUICLink(const QString &str, bool &needFix);

        // Clash parsing helpers
        void processClashProxy(const YAML::Node &proxy, const QString &type);
        void processClashShadowsocks(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent);
        void processClashSocksHttp(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent, const QString &type);
        void processClashTrojanVLESS(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent, const QString &type, bool &needFix);
        void processClashVMess(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent, bool &needFix);
        void processClashHysteria2(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent);
        void processClashTUIC(const YAML::Node &proxy, std::shared_ptr<NekoGui::ProxyEntity> &ent);
        void processClashStreamOpts(const YAML::Node &proxy, NekoGui_fmt::V2RayStreamSettings *stream);
    };

    class GroupUpdater : public QObject {
        Q_OBJECT

    public:
        void AsyncUpdate(const QString &str, int _sub_gid = -1, const std::function<void()> &finish = nullptr);

        void Update(const QString &_str, int _sub_gid = -1, bool _not_sub_as_url = false);

    signals:

        void asyncUpdateCallback(int gid);
    };

    extern GroupUpdater *groupUpdater;
} // namespace NekoGui_sub

// 更新所有订阅 关闭分组窗口时 更新动作继续执行
void UI_update_all_groups(bool onlyAllowed = false);
