#include "jshighlighter.h"

using LanguageData = QMultiHash<char, QLatin1String>;

static LanguageData js_keywords = {
    {('i'), QLatin1String("in")},
    {('o'), QLatin1String("of")},
    {('i'), QLatin1String("if")},
    {('f'), QLatin1String("for")},
    {('w'), QLatin1String("while")},
    {('f'), QLatin1String("finally")},
    {('n'), QLatin1String("new")},
    {('f'), QLatin1String("function")},
    {('d'), QLatin1String("do")},
    {('r'), QLatin1String("return")},
    {('v'), QLatin1String("void")},
    {('e'), QLatin1String("else")},
    {('b'), QLatin1String("break")},
    {('c'), QLatin1String("catch")},
    {('i'), QLatin1String("instanceof")},
    {('w'), QLatin1String("with")},
    {('t'), QLatin1String("throw")},
    {('c'), QLatin1String("case")},
    {('d'), QLatin1String("default")},
    {('t'), QLatin1String("try")},
    {('t'), QLatin1String("this")},
    {('s'), QLatin1String("switch")},
    {('c'), QLatin1String("continue")},
    {('t'), QLatin1String("typeof")},
    {('d'), QLatin1String("delete")},
    {('l'), QLatin1String("let")},
    {('y'), QLatin1String("yield")},
    {('c'), QLatin1String("const")},
    {('e'), QLatin1String("export")},
    {('s'), QLatin1String("super")},
    {('d'), QLatin1String("debugger")},
    {('a'), QLatin1String("as")},
    {('a'), QLatin1String("async")},
    {('a'), QLatin1String("await")},
    {('s'), QLatin1String("static")},
    {('i'), QLatin1String("import")},
    {('f'), QLatin1String("from")},
    {('a'), QLatin1String("as")}
};

static LanguageData js_types = {
    {('v'), QLatin1String("var")},
    {('c'), QLatin1String("class")},
    {('b'), QLatin1String("byte")},
    {('e'), QLatin1String("enum")},
    {('f'), QLatin1String("float")},
    {('s'), QLatin1String("short")},
    {('l'), QLatin1String("long")},
    {('i'), QLatin1String("int")},
    {('v'), QLatin1String("void")},
    {('b'), QLatin1String("boolean")},
    {('d'), QLatin1String("double")}
};

static LanguageData js_literals = {
    {('f'), QLatin1String("false")},
    {('n'), QLatin1String("null")},
    {('t'), QLatin1String("true")},
    {('u'), QLatin1String("undefined")},
    {('N'), QLatin1String("NaN")},
    {('I'), QLatin1String("Infinity")}
};

static LanguageData js_builtin = {
    {('e'), QLatin1String("eval")},
    {('i'), QLatin1String("isFinite")},
    {('i'), QLatin1String("isNaN")},
    {('p'), QLatin1String("parseFloat")},
    {('p'), QLatin1String("parseInt")},
    {('d'), QLatin1String("decodeURI")},
    {('d'), QLatin1String("decodeURIComponent")},
    {('d'), QLatin1String("device")},
    {('e'), QLatin1String("encodeURI")},
    {('e'), QLatin1String("encodeURIComponent")},
    {('e'), QLatin1String("escape")},
    {('u'), QLatin1String("unescape")},
    {('O'), QLatin1String("Object")},
    {('F'), QLatin1String("Function")},
    {('B'), QLatin1String("Boolean")},
    {('E'), QLatin1String("Error")},
    {('E'), QLatin1String("EvalError")},
    {('I'), QLatin1String("InternalError")},
    {('R'), QLatin1String("RangeError")},
    {('R'), QLatin1String("ReferenceError")},
    {('S'), QLatin1String("StopIteration")},
    {('S'), QLatin1String("SyntaxError")},
    {('T'), QLatin1String("TypeError")},
    {('U'), QLatin1String("URIError")},
    {('N'), QLatin1String("Number")},
    {('M'), QLatin1String("Math")},
    {('D'), QLatin1String("Date")},
    {('S'), QLatin1String("String")},
    {('R'), QLatin1String("RegExp")},
    {('A'), QLatin1String("Array")},
    {('F'), QLatin1String("Float32Array")},
    {('F'), QLatin1String("Float64Array")},
    {('I'), QLatin1String("Int16Array")},
    {('I'), QLatin1String("Int32Array")},
    {('I'), QLatin1String("Int8Array")},
    {('U'), QLatin1String("Uint16Array")},
    {('U'), QLatin1String("Uint32Array")},
    {('U'), QLatin1String("Uint8Array")},
    {('U'), QLatin1String("Uint8ClampedArray")},
    {('A'), QLatin1String("ArrayBuffer")},
    {('D'), QLatin1String("DataView")},
    {('J'), QLatin1String("JSON")},
    {('I'), QLatin1String("Intl")},
    {('a'), QLatin1String("arguments")},
    {('r'), QLatin1String("require")},
    {('m'), QLatin1String("module")},
    {('c'), QLatin1String("console")},
    {('w'), QLatin1String("window")},
    {('d'), QLatin1String("document")},
    {('S'), QLatin1String("Symbol")},
    {('S'), QLatin1String("Set")},
    {('M'), QLatin1String("Map")},
    {('W'), QLatin1String("WeakSet")},
    {('W'), QLatin1String("WeakMap")},
    {('P'), QLatin1String("Proxy")},
    {('R'), QLatin1String("Reflect")},
    {('P'), QLatin1String("Promise")}
};

static LanguageData js_other = {};

///
/// \brief JSHighlighter::JSHighlighter
/// \param parent
///
JSHighlighter::JSHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter{parent}
{
    initFormats();
}

///
/// \brief JSHighlighter::initFormats
///
void JSHighlighter::initFormats()
{
    QTextCharFormat format = QTextCharFormat();

    _formats[Token::CodeBlock] = format;
    format = QTextCharFormat();

    format.setForeground(QColor("#F92672"));
    _formats[Token::CodeKeyWord] = format;
    format = QTextCharFormat();

    format.setForeground(QColor("#a39b4e"));
    _formats[Token::CodeString] = format;
    format = QTextCharFormat();

    format.setForeground(QColor("#75715E"));
    _formats[Token::CodeComment] = format;
    format = QTextCharFormat();

    format.setForeground(QColor("#54aebf"));
    _formats[Token::CodeType] = format;

    format = QTextCharFormat();
    format.setForeground(QColor("#db8744"));
    _formats[Token::CodeOther] = format;

    format = QTextCharFormat();
    format.setForeground(QColor("#AE81FF"));
    _formats[Token::CodeNumLiteral] = format;

    format = QTextCharFormat();
    format.setForeground(QColor("#018a0f"));
    _formats[Token::CodeBuiltIn] = format;
}

///
/// \brief JSHighlighter::highlightBlock
/// \param text
///
void JSHighlighter::highlightBlock(const QString &text)
{
    if (currentBlock() == document()->firstBlock()) {
        setCurrentBlockState(0);
    } else {
        previousBlockState() == 0 ?
                    setCurrentBlockState(0) :
                    setCurrentBlockState(1);
    }

    highlightSyntax(text);
}

///
/// \brief JSHighlighter::highlightSyntax
/// \param text
///
void JSHighlighter::highlightSyntax(const QString &text)
{
    if (text.isEmpty()) return;

    const auto textLen = text.length();

    QChar comment;

    // keep the default code block format
    // this statement is very slow
    // TODO: do this formatting when necessary instead of
    // applying it to the whole block in the beginning
    setFormat(0, textLen, _formats[CodeBlock]);

    auto applyCodeFormat =
        [this](int i, const LanguageData &data,
               const QString &text, const QTextCharFormat &fmt) -> int {
        // check if we are at the beginning OR if this is the start of a word
        if (i == 0 || (!text.at(i - 1).isLetterOrNumber() &&
                       text.at(i-1) != QLatin1Char('_'))) {
            const auto wordList = data.values(text.at(i).toLatin1());
            for (const QLatin1String &word : wordList) {
                // we have a word match check
                // 1. if we are at the end
                // 2. if we have a complete word
                if (word == strMidRef(text, i, word.size()) &&
                    (i + word.size() == text.length() ||
                     (!text.at(i + word.size()).isLetterOrNumber() &&
                      text.at(i + word.size()) != QLatin1Char('_')))) {
                    setFormat(i, word.size(), fmt);
                    i += word.size();
                }
            }
        }
        return i;
    };

    const QTextCharFormat &formatType = _formats[CodeType];
    const QTextCharFormat &formatKeyword = _formats[CodeKeyWord];
    const QTextCharFormat &formatComment = _formats[CodeComment];
    const QTextCharFormat &formatNumLit = _formats[CodeNumLiteral];
    const QTextCharFormat &formatBuiltIn = _formats[CodeBuiltIn];
    const QTextCharFormat &formatOther = _formats[CodeOther];

    for (int i = 0; i < textLen; ++i) {

        if (currentBlockState() % 2 != 0) goto Comment;

        while (i < textLen && !text[i].isLetter()) {
            if (text[i].isSpace()) {
                ++i;
                //make sure we don't cross the bound
                if (i == textLen) return;
                if (text[i].isLetter()) break;
                else continue;
            }
            //inline comment
            if (comment.isNull() && text[i] == QLatin1Char('/')) {
                if((i+1) < textLen){
                    if(text[i+1] == QLatin1Char('/')) {
                        setFormat(i, textLen, formatComment);
                        return;
                    } else if(text[i+1] == QLatin1Char('*')) {
                        Comment:
                        int next = text.indexOf(QLatin1String("*/"));
                        if (next == -1) {
                            //we didn't find a comment end.
                            //Check if we are already in a comment block
                            if (currentBlockState() % 2 == 0)
                                setCurrentBlockState(currentBlockState() + 1);
                            setFormat(i, textLen,  formatComment);
                            return;
                        } else {
                            //we found a comment end
                            //mark this block as code if it was previously comment
                            //first check if the comment ended on the same line
                            //if modulo 2 is not equal to zero, it means we are in a comment
                            //-1 will set this block's state as language
                            if (currentBlockState() % 2 != 0) {
                                setCurrentBlockState(currentBlockState() - 1);
                            }
                            next += 2;
                            setFormat(i, next - i,  formatComment);
                            i = next;
                            if (i >= textLen) return;
                        }
                    }
                }
            } else if (text[i] == comment) {
                setFormat(i, textLen, formatComment);
                i = textLen;
            //integer literal
            } else if (text[i].isNumber()) {
               i = highlightNumericLiterals(text, i);
            //string literals
            } else if (text[i] == QLatin1Char('\"')) {
               i = highlightStringLiterals('\"', text, i);
            }  else if (text[i] == QLatin1Char('\'')) {
               i = highlightStringLiterals('\'', text, i);
            }
            if (i >= textLen) {
                break;
            }
            ++i;
        }

        const int pos = i;

        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Types */
        i = applyCodeFormat(i, js_types, text, formatType);
        /************************************************
         next letter is usually a space, in that case
         going forward is useless, so continue;
         We can ++i here and go to the beginning of the next word
         so that the next formatter can check for formatting but this will
         cause problems in case the next word is also of 'Type' or the current
         type(keyword/builtin). We can work around it and reset the value of i
         in the beginning of the loop to the word's first letter but I am not
         sure about its efficiency yet.
         ************************************************/
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Keywords */
        i = applyCodeFormat(i, js_keywords, text, formatKeyword);
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Literals (true/false/NULL,nullptr) */
        i = applyCodeFormat(i, js_literals, text, formatNumLit);
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Builtin library stuff */
        i = applyCodeFormat(i, js_builtin, text, formatBuiltIn);
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight other stuff (preprocessor etc.) */
        if (( i == 0 || !text.at(i-1).isLetter()) && js_other.contains(text[i].toLatin1())) {
            const QList<QLatin1String> wordList = js_other.values(text[i].toLatin1());
            for(const QLatin1String &word : wordList) {
                if (word == strMidRef(text, i, word.size()) // we have a word match
                        &&
                        (i + word.size() == text.length() // check if we are at the end
                         ||
                         !text.at(i + word.size()).isLetter()) //OR if we have a complete word
                        ) {
                                setFormat(i, word.size(), formatOther);
                    i += word.size();
                }
            }
        }

        //we were unable to find any match, lets skip this word
        if (pos == i) {
            int count = i;
            while (count < textLen) {
                if (!text[count].isLetter()) break;
                ++count;
            }
            i = count;
        }
    }
}

int JSHighlighter::highlightStringLiterals(const QChar strType, const QString &text, int i) {
    setFormat(i, 1,  _formats[CodeString]);
    ++i;

    while (i < text.length()) {
        //look for string end
        //make sure it's not an escape seq
        if (text.at(i) == strType && text.at(i-1) != QLatin1Char('\\')) {
            setFormat(i, 1,  _formats[CodeString]);
            ++i;
            break;
        }
        //look for escape sequence
        if (text.at(i) == QLatin1Char('\\') && (i+1) < text.length()) {
            int len = 0;
            switch(text.at(i+1).toLatin1()) {
            case 'a':
            case 'b':
            case 'e':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
            case '\'':
            case '"':
            case '\\':
            case '\?':
                //2 because we have to highlight \ as well as the following char
                len = 2;
                break;
            //octal esc sequence \123
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            {
                if (i + 4 <= text.length()) {
                    bool isCurrentOctal = true;
                    if (!isOctal(text.at(i+2).toLatin1())) {
                        isCurrentOctal = false;
                        break;
                    }
                    if (!isOctal(text.at(i+3).toLatin1())) {
                        isCurrentOctal = false;
                        break;
                    }
                    len = isCurrentOctal ? 4 : 0;
                }
                break;
            }
            //hex numbers \xFA
            case 'x':
            {
                if (i + 3 <= text.length()) {
                    bool isCurrentHex = true;
                    if (!isHex(text.at(i+2).toLatin1())) {
                        isCurrentHex = false;
                        break;
                    }
                    if (!isHex(text.at(i+3).toLatin1())) {
                        isCurrentHex = false;
                        break;
                    }
                    len = isCurrentHex ? 4 : 0;
                }
                break;
            }
            //TODO: implement unicode code point escaping
            default:
                break;
            }

            //if len is zero, that means this wasn't an esc seq
            //increment i so that we skip this backslash
            if (len == 0) {
                setFormat(i, 1,  _formats[CodeString]);
                ++i;
                continue;
            }

            setFormat(i, len, _formats[CodeNumLiteral]);
            i += len;
            continue;
        }
        setFormat(i, 1,  _formats[CodeString]);
        ++i;
    }
    return i;
}

///
/// \brief JSHighlighter::highlightNumericLiterals
/// \param text
/// \param i
/// \return
///
int JSHighlighter::highlightNumericLiterals(const QString &text, int i)
{
    bool isPreAllowed = false;
    if (i == 0) isPreAllowed = true;
    else {
        //these values are allowed before a number
        switch(text.at(i - 1).toLatin1()) {
        //css number
        case ':':
        case '$':
            break;
        case '[':
        case '(':
        case '{':
        case ' ':
        case ',':
        case '=':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '<':
        case '>':
            isPreAllowed = true;
            break;
        }
    }

    if (!isPreAllowed) return i;

    const int start = i;

    if ((i+1) >= text.length()) {
        setFormat(i, 1, _formats[CodeNumLiteral]);
        return ++i;
    }

    ++i;
    //hex numbers highlighting (only if there's a preceding zero)
    if (text.at(i) == QChar('x') && text.at(i - 1) == QChar('0'))
        ++i;

    while (i < text.length()) {
        if (!text.at(i).isNumber() && text.at(i) != QChar('.') &&
             text.at(i) != QChar('e')) //exponent
            break;
        ++i;
    }

    bool isPostAllowed = false;
    if (i == text.length()) {
        //cant have e at the end
        if (text.at(i - 1) != QChar('e'))
            isPostAllowed = true;
    } else {
        //these values are allowed after a number
        switch(text.at(i).toLatin1()) {
        case ']':
        case ')':
        case '}':
        case ' ':
        case ',':
        case '=':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '>':
        case '<':
        case ';':
            isPostAllowed = true;
            break;
        // for 100u, 1.0F
        case 'p':
        case 'e':
            break;
        case 'u':
        case 'l':
        case 'f':
        case 'U':
        case 'L':
        case 'F':
            if (i + 1 == text.length() || !text.at(i+1).isLetterOrNumber()) {
                isPostAllowed = true;
                ++i;
            }
            break;
        }
    }
    if (isPostAllowed) {
        int end = i;
        setFormat(start, end - start, _formats[CodeNumLiteral]);
    }
    //decrement so that the index is at the last number, not after it
    return --i;
}
