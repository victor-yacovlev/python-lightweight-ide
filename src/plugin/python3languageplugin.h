#ifndef PYTHON3_LANGUAGE_PLUGIN_H
#define PYTHON3_LANGUAGE_PLUGIN_H
//extern "C" {
//#include <Python.h>
//}

#include <QtCore>



#include <kumir2/analizerinterface.h>
#include <kumir2/runinterface.h>
#include <kumir2/startpage_widget_interface.h>
#include <kumir2-libs/extensionsystem/kplugin.h>

#include "sandboxwidget.h"

#include "pyinterpreterprocess.h"

namespace Python3Language {

struct Python3Module;
class PythonRunThread;
class PyFileHandler;
class SyntaxCheckSettingsPage;

using namespace Shared;

class Python3LanguagePlugin
        : public ExtensionSystem::KPlugin
        , public AnalizerInterface
        , public RunInterface
        , public StartpageWidgetInterface
{
    friend class PythonRunThread;
    friend class PythonAnalizerInstance;
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "kumir2.Python3Language" FILE "")
#endif
    Q_INTERFACES(Shared::AnalizerInterface Shared::RunInterface Shared::StartpageWidgetInterface)
public:
    Python3LanguagePlugin();
    ~Python3LanguagePlugin();

    // KPlugin methods
    QList<QWidget*> settingsEditorPages();

    // Analizer interface methods
    Analizer::InstanceInterface * createInstance();
    Analizer::SourceFileInterface * sourceFileHandler();
    inline bool primaryAlphabetIsLatin() const { return true; }
    inline bool caseInsensitiveGrammatic() const { return false; }
    inline IndentsBehaviour indentsBehaviour() const { return PythonIndents; }
    inline SyntaxHighlightBehaviour syntaxHighlightBehaviour() const { return RestLinesHighlight; }
    inline QString defaultDocumentFileNameSuffix() const { return "py"; }
    inline QString languageName() const { return "Python"; }

    // Runner interface methods
    inline bool hasBreakpointsSupport() const { return true; }
    bool loadProgram(const RunnableProgram & program);
    inline QDateTime loadedProgramVersion() const { return loadedProgramVersion_; }
    bool canStepOut() const;
    bool isTestingRun() const;
    bool hasMoreInstructions() const;
    bool hasTestingEntryPoint() const;
    int currentLineNo() const;
    QPair<quint32,quint32> currentColumn() const;
    QString error() const;
    QVariant valueStackTopItem() const;
    unsigned long int stepsCounted() const;
    QAbstractItemModel * debuggerVariablesViewModel() const;
    void setStdInTextStream(QTextStream *stream);
    void setStdOutTextStream(QTextStream *stream);

    // Start page interface methods
    QWidget * startPageWidget();
    QString startPageTitle() const;
    QList<QAction*> startPageActions() const;
    void setStartPageTitleChangeHandler(const QObject *, const char *);
    inline void runProgramInCurrentThread(bool useTestingEntryPoint = false) {} // Not implemented

Q_SIGNALS:
    void errorOutputRequest(const QString &);
    void outputRequest(const QString & output);
    void inputRequest(const QString & format);
    void finishInput(const QVariantList & data);
    void stopped(int reason);
    void updateStepsCounter(quint64);
    void lineChanged(int lineNo, quint32 colStart, quint32 colEnd);

public Q_SLOTS:
    // Runner interface slots
    void runBlind();
    void runContinuous();
    void runStepOver();
    void runStepInto();
    void runToEnd();
    void runTesting();
    void terminate();
    void terminateAndWaitForStopped();

    void removeAllBreakpoints();
    void insertSingleHitBreakpoint(const QString &fileName, quint32 lineNo);
    void insertOrChangeBreakpoint(bool enabled, const QString &fileName, quint32 lineNo, quint32 ignoreCount, const QString &condition);
    void removeBreakpoint(const QString &fileName, quint32 lineNo);

protected:    
    void createPluginSpec();
    void connectRunThreadSignals();
    QString initialize(const QStringList &, const ExtensionSystem::CommandLine &);
    void stop();
    static QString extractFunction(const QString & source, const QString &funcName);
    QStringList extraPaths() const;

protected Q_SLOTS:
    void updateSettings(const QStringList &);

private /*fields*/:
    PyFileHandler * _fileHandler;
//    PyThreadState * pyMain_;
    PythonRunThread * runner_;
    QDateTime loadedProgramVersion_;
    QList<class PythonAnalizerInstance*> _analizerInstances;
    SandboxWidget * _sandboxWidget;
    SyntaxCheckSettingsPage * _syntaxCheckSettingsPage;

    PyInterpreterProcess * _interpreterForAnalizers;


    // RunInterface interface
public:
    Shared::RunInterface::RunMode currentRunMode() const;
};

} // namespace

#endif
