#include "V2RayProfileGenerator.hpp"

// App Settings
#include "v2ray/app/browserforwarder/config.pb.h"
#include "v2ray/app/dns/config.pb.h"
#include "v2ray/app/dns/fakedns/fakedns.pb.h"
#include "v2ray/app/log/config.pb.h"
#include "v2ray/app/observatory/burst/config.pb.h"
#include "v2ray/app/observatory/config.pb.h"
#include "v2ray/app/observatory/multiObservatory/config.pb.h"
#include "v2ray/app/policy/config.pb.h"
#include "v2ray/app/proxyman/config.pb.h"
#include "v2ray/app/router/config.pb.h"
#include "v2ray/app/stats/config.pb.h"

// V2Ray Configuration
#include "v2ray/config.pb.h"

// Protocol Config
#include "v2ray/proxy/blackhole/config.pb.h"
#include "v2ray/proxy/dns/config.pb.h"
#include "v2ray/proxy/dokodemo/config.pb.h"
#include "v2ray/proxy/freedom/config.pb.h"
#include "v2ray/proxy/http/config.pb.h"
#include "v2ray/proxy/loopback/config.pb.h"
#include "v2ray/proxy/shadowsocks/config.pb.h"
#include "v2ray/proxy/socks/config.pb.h"
#include "v2ray/proxy/trojan/config.pb.h"
#include "v2ray/proxy/vless/account.pb.h"
#include "v2ray/proxy/vless/outbound/config.pb.h"
#include "v2ray/proxy/vmess/account.pb.h"
#include "v2ray/proxy/vmess/outbound/config.pb.h"

// Transport Config
#include "v2ray/transport/config.pb.h"
#include "v2ray/transport/internet/config.pb.h"
#include "v2ray/transport/internet/domainsocket/config.pb.h"
#include "v2ray/transport/internet/grpc/config.pb.h"
#include "v2ray/transport/internet/headers/http/config.pb.h"
#include "v2ray/transport/internet/headers/noop/config.pb.h"
#include "v2ray/transport/internet/headers/srtp/config.pb.h"
#include "v2ray/transport/internet/headers/tls/config.pb.h"
#include "v2ray/transport/internet/headers/utp/config.pb.h"
#include "v2ray/transport/internet/headers/wechat/config.pb.h"
#include "v2ray/transport/internet/headers/wireguard/config.pb.h"
#include "v2ray/transport/internet/http/config.pb.h"
#include "v2ray/transport/internet/kcp/config.pb.h"
#include "v2ray/transport/internet/quic/config.pb.h"
#include "v2ray/transport/internet/tcp/config.pb.h"
#include "v2ray/transport/internet/tls/config.pb.h"
#include "v2ray/transport/internet/udp/config.pb.h"
#include "v2ray/transport/internet/websocket/config.pb.h"

// General Headers
#include "V2RayModels.hpp"

#include <QHostAddress>

std::string to_v2ray_addr(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv4Protocol)
    {
        const auto a = addr.toIPv4Address();
        return { (char) ((a >> 24) & 0b11111111), (char) ((a >> 16) & 0b11111111), (char) ((a >> 8) & 0b11111111), (char) ((a >> 0) & 0b11111111) };
    }
    if (addr.protocol() == QAbstractSocket::IPv6Protocol)
    {
        const auto a = addr.toIPv6Address();
        return { (char) a[0], (char) a[1], (char) a[2],  (char) a[3],  (char) a[4],  (char) a[5],  (char) a[6],  (char) a[7],
                 (char) a[8], (char) a[9], (char) a[10], (char) a[11], (char) a[12], (char) a[13], (char) a[14], (char) a[15] };
    }
    assert(false);
}

void setIpOrDomin(const QString &ipOrDomain, ::v2ray::core::common::net::IPOrDomain *d)
{
    const QHostAddress addr{ ipOrDomain };
    if (addr.protocol() == QAbstractSocket::IPv4Protocol || addr.protocol() == QAbstractSocket::IPv6Protocol)
        d->set_ip(to_v2ray_addr(addr));
    else
        d->set_domain(ipOrDomain.toStdString());
}

QByteArray V2RayProfileGenerator::GenerateConfiguration(const ProfileContent &profile)
{
    v2ray::core::Config config;

    for (const auto &in : profile.inbounds)
        GenerateInboundConfig(in, config.add_inbound());

    for (const auto &out : profile.outbounds)
        GenerateOutboundConfig(out, config.add_outbound());

    return QByteArray::fromStdString(config.SerializeAsString());
}

void V2RayProfileGenerator::GenerateStreamSettings(const IOStreamSettings &s, ::v2ray::core::transport::internet::StreamConfig *vs)
{
    const auto stream = Qv2ray::Models::StreamSettingsObject::fromJson(s);

    // streamSettings.network
    vs->set_protocol_name(stream.network->toLower().toStdString());

    // streamSettings.security
    assert(stream.security->toLower() == QStringLiteral("none") || stream.security->toLower() == QStringLiteral("tls") || stream.security->isEmpty());
    vs->set_security_type(stream.security->toLower().toStdString());

    if (stream.security->toLower() == QStringLiteral("none") || stream.security->toLower().isEmpty())
        /* Do nothing */;

    if (stream.security->toLower() == QStringLiteral("tls"))
    {
        v2ray::core::transport::internet::tls::Config tlsConfig;
        tlsConfig.set_allow_insecure(false);
        tlsConfig.set_disable_system_root(stream.tlsSettings->disableSystemRoot);
        tlsConfig.set_enable_session_resumption(!stream.tlsSettings->disableSessionResumption);
        tlsConfig.set_server_name(stream.tlsSettings->serverName->toStdString());
        // tlsConfig.set_pinned_peer_certificate_chain_sha256();
        for (const auto &alpn : *stream.tlsSettings->alpn)
            tlsConfig.add_next_protocol(alpn.toStdString());

        // TODO V2Ray v5 changes the format.
        // for (const auto &cer : *stream.tlsSettings->certificates)
        // {
        //     v2ray::core::transport::internet::tls::Certificate c;
        //     c.set_key(cer.key);
        // }

        vs->add_security_settings()->PackFrom(tlsConfig);
    }

    if (stream.network->toLower() == QStringLiteral("tcp"))
    {
        v2ray::core::transport::internet::tcp::Config tcpConfig;
        if (stream.tcpSettings->header->type == QStringLiteral("none"))
            tcpConfig.mutable_header_settings()->PackFrom(v2ray::core::transport::internet::headers::noop::Config());

        if (stream.tcpSettings->header->type == QStringLiteral("http"))
        {
            v2ray::core::transport::internet::headers::http::Config http;
            const auto header = stream.tcpSettings->header;

            // Request
            http.mutable_request()->mutable_version()->set_value(header->request->version->toStdString());
            http.mutable_request()->mutable_method()->set_value(header->request->method->toStdString());
            for (const auto &p : *stream.tcpSettings->header->request->path)
                http.mutable_request()->add_uri(p.toStdString());

            for (auto it = header->request->headers->constKeyValueBegin(); it != header->request->headers->constKeyValueEnd(); it++)
            {
                auto h = http.mutable_request()->add_header();
                h->set_name(it->first.toStdString());
                for (const auto &val : it->second)
                    h->add_value(val.toStdString());
            }

            // Response
            http.mutable_response()->mutable_version()->set_value(header->response->version->toStdString());
            http.mutable_response()->mutable_status()->set_code(header->response->status->toStdString());
            http.mutable_response()->mutable_status()->set_reason(header->response->reason->toStdString());
            for (auto it = header->response->headers->constKeyValueBegin(); it != header->response->headers->constKeyValueEnd(); it++)
            {
                auto h = http.release_response()->add_header();
                h->set_name(it->first.toStdString());
                for (const auto &val : it->second)
                    h->add_value(val.toStdString());
            }
            tcpConfig.mutable_header_settings()->PackFrom(http);
        }
        vs->add_transport_settings()->mutable_settings()->PackFrom(tcpConfig);
    }

    if (stream.network->toLower() == QStringLiteral("http"))
    {
        v2ray::core::transport::internet::http::Config httpConfig;
        for (const auto &h : *stream.httpSettings->host)
            httpConfig.add_host(h.toStdString());
        httpConfig.set_path(stream.httpSettings->path->toStdString());
        httpConfig.set_method(stream.httpSettings->method->toStdString());

        // TODO HTTP Headers, Not implemented in GUI
        // httpConfig.mutable_header();

        vs->add_transport_settings()->mutable_settings()->PackFrom(httpConfig);
    }

    if (stream.network->toLower() == QStringLiteral("ws"))
    {
        v2ray::core::transport::internet::websocket::Config wsConfig;

        wsConfig.set_path(stream.wsSettings->path->toStdString());
        wsConfig.set_max_early_data(stream.wsSettings->maxEarlyData);
        wsConfig.set_use_browser_forwarding(stream.wsSettings->useBrowserForwarding);
        wsConfig.set_early_data_header_name(stream.wsSettings->earlyDataHeaderName->toStdString());
        for (const auto &[k, v] : stream.wsSettings->headers->toStdMap())
        {
            auto h = wsConfig.add_header();
            h->set_key(k.toStdString());
            h->set_value(v.toStdString());
        }

        vs->add_transport_settings()->mutable_settings()->PackFrom(wsConfig);
    }

    if (stream.network->toLower() == QStringLiteral("kcp"))
    {
        v2ray::core::transport::internet::kcp::Config kcpConfig;
        kcpConfig.mutable_mtu()->set_value(stream.kcpSettings->mtu);
        kcpConfig.mutable_tti()->set_value(stream.kcpSettings->tti);
        kcpConfig.mutable_uplink_capacity()->set_value(stream.kcpSettings->uplinkCapacity);
        kcpConfig.mutable_uplink_capacity()->set_value(stream.kcpSettings->downlinkCapacity);
        kcpConfig.set_congestion(stream.kcpSettings->congestion);
        kcpConfig.mutable_write_buffer()->set_size(stream.kcpSettings->writeBufferSize);
        kcpConfig.mutable_read_buffer()->set_size(stream.kcpSettings->readBufferSize);
        kcpConfig.mutable_seed()->set_seed(stream.kcpSettings->seed->toStdString());
#define ADD_HEADER(hdr, _type)                                                                                                                                           \
    if (stream.kcpSettings->header->type == QStringLiteral(hdr))                                                                                                         \
    kcpConfig.mutable_header_config()->PackFrom(v2ray::core::transport::internet::headers::_type{})
        ADD_HEADER("none", noop::Config);
        ADD_HEADER("srtp", srtp::Config);
        ADD_HEADER("utp", utp::Config);
        ADD_HEADER("wechat-video", wechat::VideoConfig);
        ADD_HEADER("dtls", tls::PacketConfig);
        ADD_HEADER("wireguard", wireguard::WireguardConfig);
#undef ADD_HEADER
        vs->add_transport_settings()->mutable_settings()->PackFrom(kcpConfig);
    }

    if (stream.network->toLower() == QStringLiteral("quic"))
    {
        v2ray::core::transport::internet::quic::Config quicConfig;
        quicConfig.set_key(stream.quicSettings->key->toStdString());
        // security: "none" | "aes-128-gcm" | "chacha20-poly1305"
        quicConfig.mutable_security()->set_type([&]() {
            if (stream.quicSettings->security == QStringLiteral("none"))
                return v2ray::core::common::protocol::SecurityType::NONE;
            if (stream.quicSettings->security == QStringLiteral("aes-128-gcm"))
                return v2ray::core::common::protocol::SecurityType::AES128_GCM;
            if (stream.quicSettings->security == QStringLiteral("chacha20-poly1305"))
                return v2ray::core::common::protocol::SecurityType::CHACHA20_POLY1305;
            return v2ray::core::common::protocol::SecurityType::UNKNOWN;
        }());
#define ADD_HEADER(hdr, _type)                                                                                                                                           \
    if (stream.quicSettings->header->type == QStringLiteral(hdr))                                                                                                        \
    quicConfig.mutable_header()->PackFrom(v2ray::core::transport::internet::headers::_type{})
        ADD_HEADER("none", noop::Config);
        ADD_HEADER("srtp", srtp::Config);
        ADD_HEADER("utp", utp::Config);
        ADD_HEADER("wechat-video", wechat::VideoConfig);
        ADD_HEADER("dtls", tls::PacketConfig);
        ADD_HEADER("wireguard", wireguard::WireguardConfig);
#undef ADD_HEADER
        vs->add_transport_settings()->mutable_settings()->PackFrom(quicConfig);
    }

    if (stream.network->toLower() == QStringLiteral("domainsocket"))
    {
        v2ray::core::transport::internet::domainsocket::Config dsConfig;
        dsConfig.set_padding(stream.dsSettings->padding);
        dsConfig.set_abstract(stream.dsSettings->abstract);
        dsConfig.set_path(stream.dsSettings->path->toStdString());
        vs->add_transport_settings()->mutable_settings()->PackFrom(dsConfig);
    }

    if (stream.network->toLower() == QStringLiteral("grpc"))
    {
        v2ray::core::transport::internet::grpc::encoding::Config grpcConfig;
        grpcConfig.set_service_name(stream.grpcSettings->serviceName->toStdString());
        vs->add_transport_settings()->mutable_settings()->PackFrom(grpcConfig);
    }
}

void V2RayProfileGenerator::GenerateInboundConfig(const InboundObject &in, ::v2ray::core::InboundHandlerConfig *vin)
{
    // inbound.tag
    vin->set_tag(in.name.toStdString());

    v2ray::core::app::proxyman::ReceiverConfig recv;

    setIpOrDomin(in.inboundSettings.address, recv.mutable_listen());

    // inbound.port
    recv.mutable_port_range()->set_from(in.inboundSettings.port.from);
    recv.mutable_port_range()->set_to(in.inboundSettings.port.to);

    // inbound.sniffing
    recv.mutable_sniffing_settings()->set_enabled(in.options[QStringLiteral("sniffing")][QStringLiteral("enabled")].toBool());

    // inbound.sniffing.metadataOnly
    recv.mutable_sniffing_settings()->set_metadata_only(in.options[QStringLiteral("sniffing")][QStringLiteral("metadataOnly")].toBool());

    // inbound.sniffing.destOverride
    for (const auto &str : in.options[QStringLiteral("sniffing")][QStringLiteral("destOverride")].toArray())
        recv.mutable_sniffing_settings()->add_destination_override(str.toString().toStdString());

    // TODO
    // recv.mutable_allocation_strategy();
    // recv.mutable_domain_override()

    // inbound.streamSettings
    GenerateStreamSettings(in.inboundSettings.streamSettings, recv.mutable_stream_settings());

    vin->mutable_receiver_settings()->PackFrom(recv);

    // ========================= Inbound Protocol Settings =========================

#define _in(x) in.inboundSettings.protocolSettings[QStringLiteral(x)]
    if (in.inboundSettings.protocol == QStringLiteral("http"))
    {
        using namespace v2ray::core::proxy::http;
        ServerConfig http;

        // WARN This is client object, which is used for outbounds, the existence here is just simplify the parser of users[].
        Qv2ray::Models::HTTPSOCKSObject tmpClient;
        tmpClient.loadJson(in.inboundSettings.protocolSettings);

        http.mutable_accounts()->operator[](tmpClient.user->toStdString()) = tmpClient.pass->toStdString();

        http.set_allow_transparent(_in("allowTransparent").toBool());

        // TODO UserLevel Not Implemented
        // http.set_user_level();

        // Timeout is deprecated
        // http.set_timeout(_in["timeout"].toInt());

        vin->mutable_proxy_settings()->PackFrom(http);
        return;
    }

    if (in.inboundSettings.protocol == QStringLiteral("socks"))
    {
        using namespace v2ray::core::proxy::socks;
        ServerConfig socks;
        // WARN See above for Qv2ray::Models::HttpClientObject
        Qv2ray::Models::HTTPSOCKSObject tmpClient;
        tmpClient.loadJson(in.inboundSettings.protocolSettings);

        socks.mutable_accounts()->operator[](tmpClient.user->toStdString()) = tmpClient.pass->toStdString();

        socks.mutable_address()->set_ip(_in("ip").toString().toStdString());
        socks.set_udp_enabled(_in("udp").toBool());
        socks.set_auth_type(_in("auth").toString() == QStringLiteral("noauth") ? NO_AUTH : PASSWORD);

        vin->mutable_proxy_settings()->PackFrom(socks);
    }

    if (in.inboundSettings.protocol == QStringLiteral("dokodemo-door"))
    {
        using namespace v2ray::core::proxy::dokodemo;
        Config doko;
        setIpOrDomin(_in("address").toString(), doko.mutable_address());
        doko.set_port(_in("port").toInt());
        for (const auto &n : _in("networks").toString().split(QChar::fromLatin1(',')))
            if (n == QStringLiteral("tcp"))
                doko.add_networks(v2ray::core::common::net::Network::TCP);
            else if (n == QStringLiteral("udp"))
                doko.add_networks(v2ray::core::common::net::Network::UDP);
        doko.set_follow_redirect(_in("followRedirect").toBool());

        vin->mutable_proxy_settings()->PackFrom(doko);
    }
#undef _in
    // TODO VMess, Shadowsocks, Trojan, VLESS unsupported.
}

void V2RayProfileGenerator::GenerateOutboundConfig(const OutboundObject &out, v2ray::core::OutboundHandlerConfig *vout)
{
    vout->set_tag(out.name.toStdString());
    v2ray::core::app::proxyman::SenderConfig send;
    GenerateStreamSettings(out.outboundSettings.streamSettings, send.mutable_stream_settings());

    // TODO via
    // send.mutable_via();
    send.mutable_multiplex_settings()->set_enabled(out.outboundSettings.muxSettings.enabled);
    send.mutable_multiplex_settings()->set_concurrency(out.outboundSettings.muxSettings.concurrency);
    vout->mutable_sender_settings()->PackFrom(send);

#define _out(x) out.outboundSettings.protocolSettings[QStringLiteral(x)]
    if (out.outboundSettings.protocol == QStringLiteral("blackhole"))
    {
        using namespace v2ray::core::proxy::blackhole;
        Config conf;
        if (_out("response")[QStringLiteral("type")].toString() == QStringLiteral("none"))
            conf.mutable_response()->PackFrom(NoneResponse{});
        if (_out("response")[QStringLiteral("type")].toString() == QStringLiteral("http"))
            conf.mutable_response()->PackFrom(HTTPResponse{});
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("dns"))
    {
        using namespace v2ray::core::proxy::dns;
        Config conf;
        if (!_out("network").isUndefined())
        {
            if (_out("network")[QStringLiteral("network")].toString() == QStringLiteral("tcp"))
                conf.mutable_server()->set_network(v2ray::core::common::net::TCP);
            else if (_out("network")[QStringLiteral("network")].toString() == QStringLiteral("udp"))
                conf.mutable_server()->set_network(v2ray::core::common::net::UDP);
        }

        if (!_out("address").isUndefined())
            setIpOrDomin(_out("network").toString(), conf.mutable_server()->mutable_address());
        if (!_out("port").isUndefined())
            conf.mutable_server()->set_port(_out("port").toInt());
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("freedom"))
    {
        using namespace v2ray::core::proxy::freedom;
        Config conf;
        conf.set_domain_strategy([](const QString &ds) {
            if (ds == QStringLiteral("AsIs"))
                return Config_DomainStrategy::Config_DomainStrategy_AS_IS;
            if (ds == QStringLiteral("UseIP"))
                return Config_DomainStrategy::Config_DomainStrategy_USE_IP;
            if (ds == QStringLiteral("UseIPv4"))
                return Config_DomainStrategy::Config_DomainStrategy_USE_IP4;
            if (ds == QStringLiteral("UseIPv6"))
                return Config_DomainStrategy::Config_DomainStrategy_USE_IP6;
            return Config_DomainStrategy::Config_DomainStrategy_AS_IS;
        }(_out("domainStrategy").toString(QStringLiteral("AsIs"))));

        QUrl url{ QStringLiteral("pseudo://") + _out("redirect").toString() };
        setIpOrDomin(url.host(), conf.mutable_destination_override()->mutable_server()->mutable_address());
        conf.mutable_destination_override()->mutable_server()->set_port(url.port());
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("http"))
    {
        using namespace v2ray::core::proxy::http;
        ClientConfig conf;
        Qv2ray::Models::HTTPSOCKSObject http;
        http.loadJson(out.outboundSettings.protocolSettings);

        auto s = conf.add_server();
        setIpOrDomin(out.outboundSettings.address, s->mutable_address());
        s->set_port(out.outboundSettings.port.from);

        Account acc;
        acc.set_username(http.user->toStdString());
        acc.set_password(http.pass->toStdString());
        s->add_user()->mutable_account()->PackFrom(acc);

        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("socks"))
    {
        using namespace v2ray::core::proxy::socks;
        ClientConfig conf;
        Qv2ray::Models::HTTPSOCKSObject socks;
        socks.loadJson(out.outboundSettings.protocolSettings);
        auto s = conf.add_server();
        setIpOrDomin(out.outboundSettings.address, s->mutable_address());
        s->set_port(out.outboundSettings.port.from);

        Account acc;
        acc.set_username(socks.user->toStdString());
        acc.set_password(socks.pass->toStdString());
        s->add_user()->mutable_account()->PackFrom(acc);

        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("vmess"))
    {
        using namespace v2ray::core::proxy::vmess;
        using namespace v2ray::core::proxy::vmess::outbound;
        Config conf;
        auto receiver = conf.add_receiver();

        Qv2ray::Models::VMessClientObject vmess;
        vmess.loadJson(out.outboundSettings.protocolSettings);

        setIpOrDomin(out.outboundSettings.address, receiver->mutable_address());
        receiver->set_port(out.outboundSettings.port.from);

        {
            Account acc;
            acc.set_id(vmess.id->toStdString());
            acc.set_alter_id(vmess.alterId);
            acc.mutable_security_settings()->set_type([](const QString &s) {
                if (s == QStringLiteral("aes-128-gcm"))
                    return v2ray::core::common::protocol::SecurityType::AES128_GCM;
                if (s == QStringLiteral("chacha20-poly1305"))
                    return v2ray::core::common::protocol::SecurityType::CHACHA20_POLY1305;
                if (s == QStringLiteral("auto"))
                    return v2ray::core::common::protocol::SecurityType::AUTO;
                if (s == QStringLiteral("none"))
                    return v2ray::core::common::protocol::SecurityType::NONE;
                if (s == QStringLiteral("zero"))
                    return v2ray::core::common::protocol::SecurityType::ZERO;
                return v2ray::core::common::protocol::SecurityType::AUTO;
            }(vmess.security));

            receiver->add_user()->mutable_account()->PackFrom(acc);
        }

        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("shadowsocks"))
    {
        using namespace v2ray::core::proxy::shadowsocks;
        ClientConfig conf;

        Qv2ray::Models::ShadowSocksClientObject ss;
        ss.loadJson(out.outboundSettings.protocolSettings);
        auto s = conf.add_server();
        setIpOrDomin(out.outboundSettings.address, s->mutable_address());
        s->set_port(out.outboundSettings.port.from);

        Account acc;
        acc.set_iv_check(true);
        acc.set_password(ss.password->toStdString());
        acc.set_cipher_type([](const QString &c) {
            if (c == QStringLiteral("aes-256-gcm"))
                return AES_256_GCM;
            if (c == QStringLiteral("aes-128-gcm"))
                return AES_128_GCM;
            if (c == QStringLiteral("chacha20-poly1305") || c == QStringLiteral("chacha20-ietf-poly1305"))
                return CHACHA20_POLY1305;
            if (c == QStringLiteral("none") || c == QStringLiteral("plain"))
                return NONE;
            assert(false);
        }(ss.method->toLower()));

        s->add_user()->mutable_account()->PackFrom(acc);
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("trojan"))
    {
        using namespace v2ray::core::proxy::trojan;

        ClientConfig conf;
        auto s = conf.add_server();

        setIpOrDomin(_out("address").toString(), s->mutable_address());
        s->set_port(_out("port").toInt());

        Account acc;
        acc.set_password(_out("password").toString().toStdString());
        s->add_user()->mutable_account()->PackFrom(acc);
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("vless"))
    {
        using namespace v2ray::core::proxy::vless;
        using namespace v2ray::core::proxy::vless::outbound;

        Qv2ray::Models::VLESSClientObject vless;
        vless.loadJson(out.outboundSettings.protocolSettings);

        Config conf;
        auto s = conf.add_vnext();

        setIpOrDomin(_out("address").toString(), s->mutable_address());
        s->set_port(_out("port").toInt());

        {
            Account acc;
            acc.set_id(vless.id->toStdString());
            acc.set_flow(vless.flow->toStdString());
            acc.set_encryption(vless.encryption->toStdString());
            s->add_user()->mutable_account()->PackFrom(acc);
        }
        vout->mutable_proxy_settings()->PackFrom(conf);
    }

    if (out.outboundSettings.protocol == QStringLiteral("loopback"))
    {
        using namespace v2ray::core::proxy::loopback;
        Config conf;
        conf.set_inbound_tag(_out("tag").toString().toStdString());
        vout->mutable_proxy_settings()->PackFrom(conf);
    }
#undef _out
}
