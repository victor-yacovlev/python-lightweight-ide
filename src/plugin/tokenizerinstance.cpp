#include "tokenizerinstance.h"
#include <QString>
#include <QVector>


namespace Python3Language {

TokenizerInstance::TokenizerInstance(QObject *parent, const NamesContext & globals)
    : QObject(parent)
    , _globalNames(globals)
{
    QStringList operatorsForRx = QStringList()
            << "\\(" << "\\)" << "\\[" << "\\]" << "\\{" << "\\}"
            << ":" << ";" << "\\." << "," << "#"
            << "==" << "!="
            << "\\+=" << "-=" << "/=" << "\\*=" << "\\*\\*=" << "&=" << "\\|=" << "\\*\\*"
            << "\\+" << "-" << "=" << "/" << "\\\\" << "\\*" << "&" << "\\|"
            << "\"\"\"" << "'''" << "\"" << "\'"
               ;
    _operators = QStringList()
            << "(" << ")" << "[" << "]" << "{" << "}"
            << ":" << ";" << "." << ","
            << "==" << "!="
            << "+=" << "-=" << "/=" << "*=" << "**=" << "&=" << "|=" << "**"
            << "+" << "-" << "=" << "/" << "*" << "&" << "|";
    QString operatorsPattern = operatorsForRx.join("|");
    QString numbersPattern = "\\b[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?\\b";
    QString wordsPattern = "\\b\\w+\\b";

    QString commonPattern =
            numbersPattern + "|" +
            operatorsPattern + "|" +
            wordsPattern;

    _rxNameOrNumberOrOperator = QRegExp(commonPattern);
    bool valid = _rxNameOrNumberOrOperator.isValid();
    Q_ASSERT(valid);

    _rxNameOrNumberOrOperator.setMinimal(false);

    _rxNumber = QRegExp(numbersPattern);

    _keywordsPrimary = _keywordsSecondary= QStringList()
            << "class" << "finally" << "is" << "return"
            << "continue" << "for" << "lambda" << "try"
            << "def" << "from" << "nonlocal" << "while"
            << "and" << "del" << "global" << "not"
            << "with" << "as" << "elif" << "if" << "or"
            << "yield" << "assert" << "else" << "import"
            << "pass" << "break" << "except" << "in" << "raise"
            << "async" << "await"
            ;
}

void TokenizerInstance::setSourceText(const QString &text)
{
    QStringList lines = text.split("\n");
    _lines.clear();
    Q_FOREACH(const QString &str, lines) {
        Line l;
        l.text = str;
        _lines.append(l);
    }
    for (int i=0; i<lines.size(); ++i) {
        _lines[i].lineProp = lineProp(i, _lines[i].text);
    }
}

void TokenizerInstance::setHints(const QList<SyntaxHighlightHint> &hints)
{
    _hints = hints;
}

QList<Shared::Analizer::Error> TokenizerInstance::errors() const
{
    QList<Shared::Analizer::Error> result;
    Q_FOREACH(const Line & l, _lines) {
        result.append(l.error);
    }
    return result;
}

QList<Shared::Analizer::LineProp> TokenizerInstance::lineProperties() const
{
    QList<Shared::Analizer::LineProp> result;
    Q_FOREACH(const Line & l, _lines) {
        result.append(l.lineProp);
    }
    return result;
}

QList<QPoint> TokenizerInstance::lineRanks() const
{
    QList<QPoint> result;
    Q_FOREACH(const Line & l, _lines) {
        result.append(l.rank);
    }
    return result;
}


void TokenizerInstance::takeNextToken(
        /* in params:  */ ParseMode startMode, const QString &text, int startPos,
        /* out params: */ ParseMode &endMode, int &endPos, Token &token
        ) const
{
    endMode = startMode;
    token.start = startPos;
    token.text.clear();
    token.type = Empty;

    if (startPos > text.length()) {
        endPos = startPos + 1;
        return;
    }
    if (Continue==startMode || Normal==startMode) {
        takeNextNameOrNumberOrOperatorOrComment(text, startPos, endMode, endPos, token);
        if (Continue==startMode && Comment!=token.type && text.left(startPos).trimmed().length()>0) {
            token.type = ErrorGarbageAfterBackSlash;
        }
        if (Normal == endMode || Continue == endMode) {
            return;
        }
        else {
            startPos = endPos;
            startMode = endMode;
        }
    }

    if (SingleQuotedLiteral==startMode) {
        takeStringLiteral(text, startPos, "'", false, endMode, endPos, token);
    }
    else if (DoubleQuotedLiteral==startMode) {
        takeStringLiteral(text, startPos, "\"", false, endMode, endPos, token);
    }
    else if (SingleQuotedMultiLineLiteral==startMode) {
        takeStringLiteral(text, startPos, "'''", false, endMode, endPos, token);
    }
    else if (DoubleQuotedMultilineLiteral==startMode) {
        takeStringLiteral(text, startPos, "\"\"\"", false, endMode, endPos, token);
    }
    else if (SingleQuotedRegexp==startMode) {
        takeStringLiteral(text, startPos, "'", true, endMode, endPos, token);
    }
    else if (DoubleQuotedRegexp==startMode) {
        takeStringLiteral(text, startPos, "\"", true, endMode, endPos, token);
    }
    else if (SingleQuotedMultilineRegexp==startMode) {
        takeStringLiteral(text, startPos, "'''", true, endMode, endPos, token);
    }
    else if (DoubleQuotedMultilineRegexp==startMode) {
        takeStringLiteral(text, startPos, "\"\"\"", true, endMode, endPos, token);
    }
    else {
        qWarning() << "Impossible case in " << __FILE__ << ":" << __LINE__;
    }
}

void TokenizerInstance::takeNextNameOrNumberOrOperatorOrComment(
        /* in params:  */ const QString &text, int startPos,
        /* out params: */ ParseMode &endMode, int &endPos, Token &token
        ) const
{
    int p = _rxNameOrNumberOrOperator.indexIn(text, startPos);
    if (-1 == p) {
        endPos = text.length();
        return;
    }
    token.start = p;
    token.text = _rxNameOrNumberOrOperator.cap();
    if (_operators.contains(token.text)) {
        token.type = Operator;
        endMode = Normal;
    }
    else if (-1 != _rxNumber.indexIn(token.text)) {
        token.type = Number;
        endMode = Normal;
    }
    else if ("#" == token.text) {
        token.type = Comment;
        token.text += text.mid(p+1);
    }
    else if ("\\" == token.text) {
        token.type = Operator;
        endMode = Continue;
    }
    else if ("\"\"\"" == token.text) {
        token.type = Literal;
        endMode = DoubleQuotedMultilineLiteral;
    }
    else if ("\"" == token.text) {
        token.type = Literal;
        endMode = DoubleQuotedLiteral;
    }
    else if ("'''" == token.text) {
        token.type = Literal;
        endMode = SingleQuotedMultiLineLiteral;
    }
    else if ("'" == token.text) {
        token.type = Literal;
        endMode = SingleQuotedLiteral;
    }
    else {
        token.type = Identifier;
        endMode= Normal;
    }

    if (token.start > 0 && 'r'==text[token.start-1]) {
        switch (endMode) {
        case SingleQuotedLiteral: endMode = SingleQuotedRegexp; break;
        case DoubleQuotedLiteral: endMode = DoubleQuotedRegexp; break;
        case SingleQuotedMultiLineLiteral: endMode = SingleQuotedMultilineRegexp; break;
        case DoubleQuotedMultilineLiteral: endMode = DoubleQuotedMultilineRegexp; break;
        default: break;
        }
    }

    endPos = token.start + token.text.length();
}

void TokenizerInstance::takeStringLiteral(
        /* in params:  */ const QString &text, int startPos, const QString &tokenToFind, bool rMode,
        /* out params: */ ParseMode &endMode, int &endPos, /* in/out param: */ Token &token
        ) const
{
    bool found = false;
    int p = startPos;
    while (!found && -1!=(p=text.indexOf(tokenToFind, startPos))) {
        if (! rMode && p > startPos) {
            found = text[p-1] != '\\';
        }
        else {
            found = true;
        }
    }
    if (-1==p) {
        endPos = text.length();
        token.text += text.mid(startPos);
        if ("\"\"\""!=tokenToFind && "'''"!=tokenToFind) {
            token.type = ErrorInLiteral;
            endMode = Normal;
        }
        else if ("\"\"\""==tokenToFind) {
            token.type = Literal;
            endMode = DoubleQuotedMultilineLiteral;
        }
        else if ("'''"==tokenToFind) {
            token.type = Literal;
            endMode = SingleQuotedMultiLineLiteral;
        }
    }
    else {
        endPos = p + tokenToFind.length();
        token.text += text.mid(startPos, endPos-startPos);
        token.type = Literal;
        endMode = Normal;
    }

}

void TokenizerInstance::detectIdentifierType(Token &token, int lineNo, const QList<Token>& prevTokens) const
{
    if (Identifier == token.type && _keywordsPrimary.contains(token.text)) {
       token.type = Keyword;
    }

    // Use trivial assumptions
    if (Identifier == token.type) {
        if (prevTokens.size() > 0 && prevTokens.last().type==Keyword && prevTokens.last().text=="def") {
            token.type = FunctionName;
        }
        else if (prevTokens.size() > 0 && prevTokens.last().type==Keyword && prevTokens.last().text=="from") {
            token.type = ModuleName;
        }
    }

    // Check for hints provided by Python ast node visiton
    if (Identifier == token.type) {
        token.type = findHintForTokenIdentifier(token.text, lineNo);
    }

    // Apply global names (might be overridden, so check AFTER compiler hints
    if (Identifier == token.type && _globalNames.functions.contains(token.text)) {
        token.type = FunctionName;
    }
    if (Identifier == token.type && _globalNames.classes.contains(token.text)) {
        token.type = ClassName;
    }
    if (Identifier == token.type && _globalNames.modules.contains(token.text)) {
        token.type = ModuleName;
    }
}

TokenType TokenizerInstance::findHintForTokenIdentifier(const QString &name, int lineNo) const
{
    Q_FOREACH(const SyntaxHighlightHint &hint, _hints) {
        if (hint.line == lineNo && hint.name == name)
            return hint.type;
    }
    return Identifier;
}

void TokenizerInstance::updateLinePropFromTokens(TokenizerInstance::Line &line) const
{
    Q_FOREACH(const Token &t, line.tokens) {
        int start = t.start;
        int end = start + t.text.length();
        start = qMin(start, line.text.length()-1);
        start = qMax(0, start);
        end = qMax(0, end);
        end = qMin(end, line.text.length());
        for (int i=start; i<end; ++i) {
            using namespace Shared;
            LexemType & ref = line.lineProp[i];
            switch (t.type) {
            case Identifier:
                break;
            case Operator:
                ref = LxTypeOperator;
                break;
            case Number:
                ref = LxConstReal;
                break;
            case Literal:
                ref = LxConstLiteral;
                break;
            case MultiLine:
                ref = LxConstLiteral;
                break;
            case Comment:
                ref = LxTypeComment;
                break;
            case LineCont:
                ref = LxTypeOperator;
                break;
            case Keyword:
                ref = LxTypePrimaryKwd;
                break;
            case ModuleName:
                ref = LxNameModule;
                break;
            case ClassName:
                ref = LxNameClass;
                break;
            case FunctionName:
                ref = LxNameAlg;
                break;
            case ConstantName:
                ref = LxTypeConstant;
                break;
            case Empty:
                break;
            case ErrorInLiteral:
                ref = LexemType(LxConstLiteral | LxTypeError);
                break;
            case ErrorGarbageAfterBackSlash:
                ref = LxTypeError;
                break;

            }
        }
    }
}

Shared::Analizer::LineProp TokenizerInstance::lineProp(int lineNo, const QString &text) const
{
    if (_lines.size() <= lineNo ) {
        // Not ready yet, so append empty lines
        for (int i=_lines.size(); i<lineNo+1; ++i) {
            _lines.append(Line());
        }
    }
    ParseMode mode = lineNo > 0
            ? _lines[lineNo-1].parseModeAtEnd
            : Normal;
    Shared::Analizer::LineProp & lp = _lines[lineNo].lineProp;
    lp.resize(text.length());
    lp.fill(Shared::LxTypeEmpty, text.length());
    _lines[lineNo].tokens.clear();

    int startPos = 0;
    int endPos = 1;
    ParseMode endMode = mode;
    Token token;
    while (startPos < text.length()) {
        takeNextToken(mode, text, startPos, endMode, endPos, token);
        detectIdentifierType(token, lineNo, _lines[lineNo].tokens);
        _lines[lineNo].tokens.append(token);
        mode = endMode;
        startPos = endPos;
    }
    _lines[lineNo].text = text;
    _lines[lineNo].parseModeAtEnd = endMode;
    updateLinePropFromTokens(_lines[lineNo]);
    return lp;
}


} // namespace Python3Language
