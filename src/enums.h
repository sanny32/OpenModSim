#ifndef ENUMS_H
#define ENUMS_H

#include <QMetaType>
#include <QMetaEnum>
#include <QSettings>

template<typename Enum>
struct EnumStrings {
    static const QMap<Enum, QString>& mapping() = delete; // Use DECLARE_ENUM_STRINGS() macro
};

#define DECLARE_ENUM_STRINGS(EnumType, ...) \
template<> \
    struct EnumStrings<EnumType> { \
        static const QMap<EnumType, QString>& mapping() { \
            static const QMap<EnumType, QString> map = { __VA_ARGS__ }; \
            return map; \
    } \
};

///
/// \brief enumToString
/// \param value
/// \return
///
template<typename Enum>
inline QString enumToString(Enum value) {
    const auto& map = EnumStrings<Enum>::mapping();
    if (auto it = map.find(value); it != map.end())
        return it.value();
    return QString::number(static_cast<int>(value));
}

///
/// \brief enumFromString
/// \param str
/// \param defaultValue
/// \return
///
template<typename Enum>
inline Enum enumFromString(const QString& str, Enum defaultValue = static_cast<Enum>(0)) {
    const auto& map = EnumStrings<Enum>::mapping();
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().compare(str, Qt::CaseInsensitive) == 0)
            return it.key();
    }
    bool ok;
    int val = str.toInt(&ok);
    if (ok)
        return static_cast<Enum>(val);
    return defaultValue;
}

///
/// \brief The AddressBase enum
///
enum class AddressBase
{
    Base0 = 0,
    Base1
};
Q_DECLARE_METATYPE(AddressBase)
DECLARE_ENUM_STRINGS(AddressBase,
                {   AddressBase::Base0, "Base0" },
                {   AddressBase::Base1, "Base1" }
)

///
/// \brief operator <<
/// \param out
/// \param base
/// \return
///
inline QSettings& operator <<(QSettings& out, const AddressBase& base)
{
    out.setValue("AddressBase", (uint)base);
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param base
/// \return
///
inline QSettings& operator >>(QSettings& in, AddressBase& base)
{
    base = (AddressBase)in.value("AddressBase").toUInt();
    return in;
}

///
/// \brief The AddressSpace enum
///
enum class AddressSpace
{
    Addr6Digits = 0,
    Addr5Digits
};
Q_DECLARE_METATYPE(AddressSpace)
DECLARE_ENUM_STRINGS(AddressSpace,
                {   AddressSpace::Addr6Digits, "6-Digits"    },
                {   AddressSpace::Addr5Digits, "5-Digits"    }
)

///
/// \brief operator <<
/// \param out
/// \param asp
/// \return
///
inline QSettings& operator <<(QSettings& out, const AddressSpace& asp)
{
    out.setValue("AddressSpace", (uint)asp);
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param asp
/// \return
///
inline QSettings& operator >>(QSettings& in, AddressSpace& asp)
{
    asp = (AddressSpace)in.value("AddressSpace").toUInt();
    return in;
}

///
/// \brief The DataType enum
///
enum class DataType
{
    Binary = 0,
    UInt16,
    Int16,
    Hex,
    Float32,
    Float64,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Ansi
};
Q_DECLARE_METATYPE(DataType)
DECLARE_ENUM_STRINGS(DataType,
                {   DataType::Binary,   "Binary"  },
                {   DataType::UInt16,   "UInt16"  },
                {   DataType::Int16,    "Int16"   },
                {   DataType::Hex,      "Hex"     },
                {   DataType::Float32,  "Float32" },
                {   DataType::Float64,  "Float64" },
                {   DataType::Int32,    "Int32"   },
                {   DataType::UInt32,   "UInt32"  },
                {   DataType::Int64,    "Int64"   },
                {   DataType::UInt64,   "UInt64"  },
                {   DataType::Ansi,     "Ansi"    },
)

///
/// \brief operator <<
/// \param out
/// \param type
/// \return
///
inline QSettings& operator <<(QSettings& out, const DataType& type)
{
    out.setValue("DataType", enumToString(type));
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param type
/// \return
///
inline QSettings& operator >>(QSettings& in, DataType& type)
{
    type = enumFromString<DataType>(in.value("DataType").toString(), DataType::UInt16);
    return in;
}

///
/// \brief The RegisterOrder enum
///
enum class RegisterOrder
{
    MSRF = 0,  // Most Significant Register First
    LSRF       // Least Significant Register First
};
Q_DECLARE_METATYPE(RegisterOrder)
DECLARE_ENUM_STRINGS(RegisterOrder,
                {   RegisterOrder::MSRF, "MSRF" },
                {   RegisterOrder::LSRF, "LSRF" },
)

///
/// \brief operator <<
/// \param out
/// \param order
/// \return
///
inline QSettings& operator <<(QSettings& out, const RegisterOrder& order)
{
    out.setValue("RegisterOrder", enumToString(order));
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param order
/// \return
///
inline QSettings& operator >>(QSettings& in, RegisterOrder& order)
{
    order = enumFromString<RegisterOrder>(in.value("RegisterOrder").toString(), RegisterOrder::MSRF);
    return in;
}

///
/// \brief isMultiRegisterType
/// \param type
/// \return
///
inline static bool isMultiRegisterType(DataType type)
{
    switch(type)
    {
        case DataType::Float32:
        case DataType::Float64:
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Int64:
        case DataType::UInt64:
            return true;
        default:
            return false;
    }
}

///
/// \brief registersCount
/// \param type
/// \return
///
inline static int registersCount(DataType type)
{
    switch(type)
    {
        case DataType::Float32:
        case DataType::Int32:
        case DataType::UInt32:
            return 2;

        case DataType::Float64:
        case DataType::Int64:
        case DataType::UInt64:
            return 4;

        default:
            return 1;
    }
}

///
/// \brief The ByteOrder enum
///
enum class ByteOrder
{
    Direct = 0,
    Swapped
};
Q_DECLARE_METATYPE(ByteOrder);
DECLARE_ENUM_STRINGS(ByteOrder,
                {   ByteOrder::Direct,    "Direct"    },
                {   ByteOrder::Swapped,   "Swapped"   }
)

///
/// \brief operator <<
/// \param out
/// \param order
/// \return
///
inline QSettings& operator <<(QSettings& out, const ByteOrder& order)
{
    out.setValue("ByteOrder", static_cast<uint>(order));
    return out;
}

///
/// \brief operator >>
/// \param in
/// \param order
/// \return
///
inline QSettings& operator >>(QSettings& in, ByteOrder& order)
{
    order = (ByteOrder)in.value("ByteOrder").toUInt();
    return in;
}

///
/// \brief The ConnectionType enum
///
enum class ConnectionType
{
    Tcp = 0,
    Serial
};
Q_DECLARE_METATYPE(ConnectionType)
DECLARE_ENUM_STRINGS(ConnectionType,
                {   ConnectionType::Tcp,      "Tcp"    },
                {   ConnectionType::Serial,   "Serial" }
)

///
/// \brief The TransmissionMode enum
///
enum class TransmissionMode
{
    ASCII = 0,
    RTU
};
Q_DECLARE_METATYPE(TransmissionMode)
DECLARE_ENUM_STRINGS(TransmissionMode,
                {   TransmissionMode::ASCII, "ASCII"  },
                {   TransmissionMode::RTU,   "RTU"    }
)

///
/// \brief The SimulationMode enum
///
enum class SimulationMode
{
    Disabled = 0,
    Off,
    Random,
    Increment,
    Decrement,
    Toggle
};
Q_DECLARE_METATYPE(SimulationMode)
DECLARE_ENUM_STRINGS(SimulationMode,
                {   SimulationMode::Disabled,    "Disabled"     },
                {   SimulationMode::Off,         "Off"          },
                {   SimulationMode::Random,      "Random"       },
                {   SimulationMode::Increment,   "Increment"    },
                {   SimulationMode::Decrement,   "Decrement"    },
                {   SimulationMode::Toggle,      "Toggle"       }
)

///
/// \brief The RunMode enum
///
enum class RunMode
{
    Once = 0,
    Periodically
};
Q_DECLARE_METATYPE(RunMode)
DECLARE_ENUM_STRINGS(RunMode,
                {   RunMode::Once,         "Once"           },
                {   RunMode::Periodically, "Periodically"   }
)

///
/// \brief The LogViewState enum
///
enum class LogViewState {
    Unknown,
    Running,
    Paused
};
Q_DECLARE_METATYPE(LogViewState)
DECLARE_ENUM_STRINGS(LogViewState,
                {   LogViewState::Unknown,    "Unknown"   },
                {   LogViewState::Running,    "Running"   },
                {   LogViewState::Paused,     "Paused"    }
)

///
/// \brief boolToString
/// \param value
/// \return
///
inline QString boolToString(bool value)
{
    return value ? "true" : "false";
}

///
/// \brief stringToBool
/// \param str
/// \return
///
inline bool stringToBool(const QString& str)
{
    const QString lower = str.toLower();
    return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
}

#endif // ENUMS_H
