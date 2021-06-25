#include "GeositeReader.hpp"

#include "Qv2rayBaseFeatures.hpp"
#include "picoproto.h"

#include <QFile>
#include <QMap>

#define QV_MODULE_NAME "GeositeReader"

namespace Qv2ray::components::GeositeReader
{
    QMap<QString, QStringList> GeositeEntries;

    QStringList ReadGeoSiteFromFile(const QString &filepath, bool allowCache)
    {
        if (GeositeEntries.contains(filepath) && allowCache)
            return GeositeEntries.value(filepath);

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
            picoproto::Message root;
            root.ParseFromBytes((unsigned char *) content.data(), content.size());

            list.reserve(root.GetMessageArray(1).size());
            for (const auto &geosite : root.GetMessageArray(1))
                list << QString::fromStdString(geosite->GetString(1));
        }

        QvLog() << "Loaded" << list.count() << "geosite entries from data file.";
        list.sort();
        GeositeEntries[filepath] = list;
        return list;
    }
} // namespace Qv2ray::components::geosite
