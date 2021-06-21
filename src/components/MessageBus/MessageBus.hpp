#pragma once
#include <QObject>

#define QvMessageBusConnect() connect(UIMessageBus, &MessageBus::QvMessageBusObject::QvSendMessage, this, &std::remove_reference_t<decltype(*this)>::on_QvMessageReceived)

#define QvMessageBusSlotSig const Qv2ray::components::MessageBus::QvMBMessage &msg
#define QvMessageBusSlotIdentifier on_QvMessageReceived

#define QvMessageBusSlotDecl void QvMessageBusSlotIdentifier(QvMessageBusSlotSig)
#define QvMessageBusSlotImpl(CLASSNAME) void CLASSNAME::QvMessageBusSlotIdentifier(QvMessageBusSlotSig)

#define MBShowDefaultImpl                                                                                                                                                \
    case Qv2ray::components::MessageBus::SHOW_WINDOWS:                                                                                                                   \
    {                                                                                                                                                                    \
        this->setWindowOpacity(1);                                                                                                                                       \
        break;                                                                                                                                                           \
    }

#define MBHideDefaultImpl                                                                                                                                                \
    case Qv2ray::components::MessageBus::HIDE_WINDOWS:                                                                                                                   \
    {                                                                                                                                                                    \
        this->setWindowOpacity(0);                                                                                                                                       \
        break;                                                                                                                                                           \
    }

#define MBRetranslateDefaultImpl                                                                                                                                         \
    case Qv2ray::components::MessageBus::RETRANSLATE:                                                                                                                    \
    {                                                                                                                                                                    \
        this->retranslateUi(this);                                                                                                                                       \
        break;                                                                                                                                                           \
    }

#define MBUpdateColorSchemeDefaultImpl                                                                                                                                   \
    case Qv2ray::components::MessageBus::UPDATE_COLORSCHEME:                                                                                                             \
    {                                                                                                                                                                    \
        this->updateColorScheme();                                                                                                                                       \
        break;                                                                                                                                                           \
    }

namespace Qv2ray::components::MessageBus
{
    Q_NAMESPACE
    enum QvMBMessage
    {
        /// Show all windows.
        SHOW_WINDOWS,
        /// Hide all windows.
        HIDE_WINDOWS,
        /// Retranslate User Interface.
        RETRANSLATE,
        /// Change Color Scheme
        UPDATE_COLORSCHEME
    };
    Q_ENUM_NS(QvMBMessage)

    class QvMessageBusObject : public QObject
    {
        Q_OBJECT
      public:
        explicit QvMessageBusObject(){};
        void EmitGlobalSignal(const QvMBMessage &msg)
        {
            emit QvSendMessage(msg);
        }
      signals:
        void QvSendMessage(const QvMBMessage &msg);
    };

} // namespace Qv2ray::components::MessageBus

inline Qv2ray::components::MessageBus::QvMessageBusObject *UIMessageBus;
