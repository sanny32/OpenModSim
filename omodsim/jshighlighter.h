#ifndef JSHIGHLIGHTER_H
#define JSHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringView>
#endif

using LanguageData = QMultiHash<char, QLatin1String>;

class JSHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum Token {
        CodeBlock,
        CodeKeyWord,
        CodeString,
        CodeComment,
        CodeType,
        CodeOther,
        CodeNumLiteral,
        CodeBuiltIn,
    };
    Q_ENUM(Token)

    explicit JSHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
private:
    void highlightSyntax(const QString &text);
    Q_REQUIRED_RESULT int highlightNumericLiterals(const QString &text, int i);
    Q_REQUIRED_RESULT int highlightStringLiterals(const QChar strType, const QString &text, int i);

    /**
     * @brief returns true if c is octal
     * @param c the char being checked
     * @returns true if the number is octal, false otherwise
     */
    Q_REQUIRED_RESULT static constexpr inline bool isOctal(const char c) {
        return (c >= '0' && c <= '7');
    }

    /**
     * @brief returns true if c is hex
     * @param c the char being checked
     * @returns true if the number is hex, false otherwise
     */
    Q_REQUIRED_RESULT static constexpr inline bool isHex(const char c) {
        return (
            (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F')
        );
    }

    void initFormats();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    static inline QStringView strMidRef(const QString& str, qsizetype position, qsizetype n = -1)
    {
        return QStringView(str).mid(position, n);
    }
#else
    static inline QStringRef strMidRef(const QString& str, int position, int n = -1)
    {
        return str.midRef(position, n);
    }
#endif

    QHash<Token, QTextCharFormat> _formats;

private:
    static LanguageData keywords;
    static LanguageData types;
    static LanguageData literals;
    static LanguageData builtin;
    static LanguageData other;
};

#endif // JSHIGHLIGHTER_H
