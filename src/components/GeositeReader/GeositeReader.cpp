#include "GeositeReader.hpp"

#include "Qv2rayBaseFeatures.hpp"
#include "v2ray_geosite.pb.h"

#include <QFile>
#include <QMap>

#define QV_MODULE_NAME "GeositeReader"

namespace Qv2ray::components::GeositeReader
{
    QMap<QString, QStringList> GeositeEntries;
    QStringList ReadGeoSiteFromFile(const QString &filepath)
    {
        if (GeositeEntries.contains(filepath))
        {
            return GeositeEntries.value(filepath);
        }
        else
        {
            QStringList list;
            QvLog() << "Reading geosites from:" << filepath;
            QFile f(filepath);
            bool opened = f.open(QFile::OpenModeFlag::ReadOnly);

            if (!opened)
            {
                QvLog() << "File cannot be opened:" << filepath;
                return list;
            }

            const auto content = f.readAll();
            f.close();

            {
                GOOGLE_PROTOBUF_VERIFY_VERSION;
                v2ray::core::app::router::GeoSiteList sites;
                sites.ParseFromArray(content.data(), content.size());
                list.reserve(sites.entry().size());
                for (const auto &e : sites.entry())
                {
                    // We want to use lower string.
                    list << e.country_code().c_str();
                }
                // Optional:  Delete all global objects allocated by libprotobuf.
                google::protobuf::ShutdownProtobufLibrary();
            }

            QvLog() << "Loaded" << list.count() << "geosite entries from data file.";
            list.sort();
            GeositeEntries[filepath] = list;
            return list;
        }
    }
} // namespace Qv2ray::components::geosite
