#include "componentinfocontrol.h"

///
/// \brief ComponentInfoControl::ComponentInfoControl
/// \param parent
///
ComponentInfoControl::ComponentInfoControl(QWidget *parent)
    : QWidget(parent)
    ,_titleLabel(new QLabel(this))
    ,_versionLabel(new QLabel(this))
    ,_descriptionLabel(new QLabel(this))
    ,_underline(new QFrame(this))
    ,_linkButton(new QPushButton(this))
    ,_linkUrl()
{
    setupUI();
    setLinkIcon(QIcon::fromTheme("applications-internet"));
}

///
/// \brief ComponentInfoControl::setupUI
///
void ComponentInfoControl::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    auto infoLayout = new QHBoxLayout();
    auto textLayout = new QVBoxLayout();
    auto titleLayout = new QHBoxLayout();

    mainLayout->setSpacing(4);
    textLayout->setSpacing(2);
    titleLayout->setSpacing(4);

    QFont titleFont = _titleLabel->font();
    titleFont.setBold(true);
    _titleLabel->setFont(titleFont);

    QPalette versionPalette = _versionLabel->palette();
    versionPalette.setColor(QPalette::WindowText, Qt::darkGray);
    _versionLabel->setPalette(versionPalette);

    _linkButton->setCursor(Qt::PointingHandCursor);
    _linkButton->setFlat(true);
    _linkButton->setFixedSize(20, 20);
    _linkButton->setIconSize(QSize(16, 16));

    _underline->setFrameShape(QFrame::HLine);
    _underline->setFrameShadow(QFrame::Sunken);
    _underline->setLineWidth(1);
    _underline->setMidLineWidth(0);

    connect(_linkButton, &QPushButton::clicked, this, &ComponentInfoControl::on_linkClicked);

    titleLayout->addWidget(_titleLabel);
    titleLayout->addWidget(_versionLabel);
    titleLayout->addStretch();

    textLayout->addLayout(titleLayout);
    textLayout->addWidget(_descriptionLabel);

    infoLayout->addLayout(textLayout);
    infoLayout->addWidget(_linkButton);

    _descriptionLabel->setWordWrap(true);

    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(_underline);

    _linkButton->setVisible(false);
}

///
/// \brief ComponentInfoControl::setTitle
/// \param title
///
void ComponentInfoControl::setTitle(const QString &title)
{
    _titleLabel->setText(title);
}

///
/// \brief ComponentInfoControl::setVersion
/// \param version
///
void ComponentInfoControl::setVersion(const QString &version)
{
    _versionLabel->setText(QString("(%1)").arg(version));
}

///
/// \brief ComponentInfoControl::setDescription
/// \param description
///
void ComponentInfoControl::setDescription(const QString &description)
{
    _descriptionLabel->setText(description);
}

///
/// \brief ComponentInfoControl::setLinkUrl
/// \param url
///
void ComponentInfoControl::setLinkUrl(const QUrl &url)
{
    _linkUrl = url;
    updateLinkButton();
}

///
/// \brief ComponentInfoControl::setLinkIcon
/// \param icon
///
void ComponentInfoControl::setLinkIcon(const QIcon &icon)
{
    _linkButton->setIcon(icon);
}

///
/// \brief ComponentInfoControl::setLinkToolTip
/// \param toolTip
///
void ComponentInfoControl::setLinkToolTip(const QString &toolTip)
{
    _linkButton->setToolTip(toolTip);
}

///
/// \brief ComponentInfoControl::title
/// \return
///
QString ComponentInfoControl::title() const
{
    return _titleLabel->text();
}

///
/// \brief ComponentInfoControl::version
/// \return
///
QString ComponentInfoControl::version() const
{
    QString versionText = _versionLabel->text();
    if (versionText.startsWith('(') && versionText.endsWith(')')) {
        return versionText.mid(1, versionText.length() - 2);
    }
    return versionText;
}

///
/// \brief ComponentInfoControl::description
/// \return
///
QString ComponentInfoControl::description() const
{
    return _descriptionLabel->text();
}

///
/// \brief ComponentInfoControl::linkUrl
/// \return
///
QUrl ComponentInfoControl::linkUrl() const
{
    return _linkUrl;
}

///
/// \brief ComponentInfoControl::linkToolTip
/// \return
///
QString ComponentInfoControl::linkToolTip() const
{
    return _linkButton->toolTip();
}

///
/// \brief ComponentInfoControl::on_linkClicked
///
void ComponentInfoControl::on_linkClicked()
{
    if (_linkUrl.isValid()) {
        QDesktopServices::openUrl(_linkUrl);
    }
}

///
/// \brief ComponentInfoControl::updateLinkButton
///
void ComponentInfoControl::updateLinkButton()
{
    const bool shouldBeVisible = _linkUrl.isValid() && !_linkUrl.isEmpty();
    _linkButton->setVisible(shouldBeVisible);
    _linkButton->setToolTip(_linkUrl.toString());
}
