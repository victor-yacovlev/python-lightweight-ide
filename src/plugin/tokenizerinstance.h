#ifndef PYTHON3LANGUAGE_TOKENIZERINSTANCE_H
#define PYTHON3LANGUAGE_TOKENIZERINSTANCE_H

#include <kumir2/analizer_instanceinterface.h>
#include "namecontext.h"
#include <QObject>
#include <QString>
#include <QRegExp>

/* Tokenizer made as C++ implementation by performance reasons */

namespace Python3Language {

enum TokenType {
    Identifier, Operator, Number, Literal, MultiLine, Comment, LineCont,
    Keyword, ModuleName, ClassName, FunctionName, ConstantName, Empty, ErrorInLiteral, ErrorGarbageAfterBackSlash
};

struct SyntaxHighlightHint {
    QString name;
    TokenType type;
    int line;
};

class TokenizerInstance : public QObject
{
    Q_OBJECT
public:
    explicit TokenizerInstance(QObject *parent, const NamesContext & globals);
    void setSourceText(const QString &text);
    void setHints(const QList<SyntaxHighlightHint> & hints);
    QList<Shared::Analizer::Error> errors() const;
    QList<Shared::Analizer::LineProp> lineProperties() const;
    QList<QPoint> lineRanks() const;
    Shared::Analizer::LineProp lineProp(int lineNo, const QString &text) const;


private:    

    struct Token {
        QString text;
        TokenType type;
        int start;
    };

    enum ParseMode {
        Normal,
        Continue,
        SingleQuotedLiteral,
        DoubleQuotedLiteral,
        SingleQuotedMultiLineLiteral,
        DoubleQuotedMultilineLiteral,
        SingleQuotedRegexp,
        DoubleQuotedRegexp,
        SingleQuotedMultilineRegexp,
        DoubleQuotedMultilineRegexp,
        CommentToEndOfLine
    };

    struct Line {
        QString text;
        Shared::Analizer::Error error;
        Shared::Analizer::LineProp lineProp;
        QPoint rank;
        QList<Token> tokens;
        ParseMode parseModeAtEnd = Normal;
    };

    void takeNextToken(
            /* in params:  */ ParseMode startMode, const QString &text, int startPos,
            /* out params: */ ParseMode &endMode, int &endPos, Token &token
            ) const;
    void takeNextNameOrNumberOrOperatorOrComment(
            /* in params:  */ const QString &text, int startPos,
            /* out params: */ ParseMode &endMode, int &endPos, Token &token
            ) const;
    void takeStringLiteral(
            /* in params:  */ const QString &text, int startPos, const QString &tokenToFind, bool rMode,
            /* out params: */ ParseMode &endMode, int &endPos, /* in/out param: */ Token &token
            ) const;

    void detectIdentifierType(Token &token, int lineNo, const QList<Token> &prevTokens) const;
    TokenType findHintForTokenIdentifier(const QString &name, int lineNo) const;
    void updateLinePropFromTokens(Line &line) const;

    mutable QList<Line> _lines;
    const NamesContext & _globalNames;
    QStringList _operators;
    QStringList _keywordsPrimary;
    QStringList _keywordsSecondary;
    mutable QRegExp _rxNameOrNumberOrOperator;
    mutable QRegExp _rxNumber;
    mutable QList<SyntaxHighlightHint> _hints;
};

} // namespace Python3Language

#endif // PYTHON3LANGUAGE_TOKENIZERINSTANCE_H
