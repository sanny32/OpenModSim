#ifndef MACAPPSTYLE_H
#define MACAPPSTYLE_H

#if defined(HAVE_QLEMENTINE_APP_STYLE)

#include "qlementineappstyle.h"

///
/// \brief The MacAppStyle class
///
class MacAppStyle : public QlementineAppStyle
{
public:
    explicit MacAppStyle(QObject* parent = nullptr);
};

#endif // HAVE_QLEMENTINE_APP_STYLE
#endif // MACAPPSTYLE_H
