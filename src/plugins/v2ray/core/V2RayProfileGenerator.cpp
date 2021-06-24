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
#include <arpa/inet.h>

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
    {
        // Do nothing
    }

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
    const QHostAddress addr{ in.listenAddress };
    const auto ip2v2rayaddr = [](const QHostAddress &addr) -> std::string {
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
    };

    // inbound.listen
    if (addr.protocol() == QAbstractSocket::IPv4Protocol || addr.protocol() == QAbstractSocket::IPv6Protocol)
        recv.mutable_listen()->set_ip(ip2v2rayaddr(addr));
    else
        recv.mutable_listen()->set_domain(in.listenAddress.toStdString());

    // inbound.port
    recv.mutable_port_range()->set_from(in.listenPort.from);
    recv.mutable_port_range()->set_to(in.listenPort.to);

    // inbound.sniffing
    recv.mutable_sniffing_settings()->set_enabled(in.options[QStringLiteral("sniffing")][QStringLiteral("enabled")].toBool());

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

    if (in.inboundSettings.protocol == QStringLiteral("http"))
    {
        v2ray::core::proxy::http::ServerConfig http;
        // http.set_allow_transparent();
        // http.set_user_level()
        // http.set_timeout()
        vin->mutable_proxy_settings()->PackFrom(http);
        return;
    }

    if (in.inboundSettings.protocol == QStringLiteral("socks"))
    {
    }
    if (in.inboundSettings.protocol == QStringLiteral("dokodemo-door"))
    {
    }

    // TODO VMess, Shadowsocks, Trojan, VLESS unsupported.
}

void V2RayProfileGenerator::GenerateOutboundConfig(const OutboundObject &, v2ray::core::OutboundHandlerConfig *)
{
}
