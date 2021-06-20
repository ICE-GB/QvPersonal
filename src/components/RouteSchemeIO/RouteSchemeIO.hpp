#pragma once

#include "models/V2RayModels.hpp"

#include <QObject>

namespace Qv2ray::components::RouteSchemeIO
{
    const inline Qv2ray::Models::RouteMatrixConfig emptyScheme;
    const inline Qv2ray::Models::RouteMatrixConfig noAdsScheme({ {}, { "geosite:category-ads-all" }, {} }, { {}, {}, {} }, "AsIs");

    /**
     * @brief The Qv2rayRouteScheme struct
     * @author DuckSoft <realducksoft@gmail.com>
     */
    struct Qv2rayRouteScheme : Qv2ray::Models::RouteMatrixConfig
    {

        // M: all these fields are mandatory
        QJS_FUNC_JSON(F(name, author, description), B(RouteMatrixConfig))
        /**
         * @brief the name of the scheme.
         * @example "Untitled Scheme"
         */
        Bindable<QString> name;
        /**
         * @brief the author of the scheme.
         * @example "DuckSoft <realducksoft@gmail.com>"
         */
        Bindable<QString> author;
        /**
         * @brief details of this scheme.
         * @example "A scheme to bypass China mainland, while allowing bilibili to go through proxy."
         */
        Bindable<QString> description;
    };
} // namespace Qv2ray::components::RouteSchemeIO
