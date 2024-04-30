#ifndef APPLICATION_H
#define APPLICATION_H
#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QVector>

class Application
{
public:
    Application(std::istream* inputStream,
                std::ostream* outputStream,
                std::ostream* errorStream,
                QCoreApplication* qCoreApplication,
                const bool& debugIsEnabledValue = false);
    enum OnFileRepeatAction {
        overwrite,
        createNewFileWithCounterInFilename
    };

    class UserSettings {
    public:
        UserSettings(std::istream* inputStream,
                     std::ostream* outputStream,
                     std::ostream* errorStream);
        void readUserSettings();

    private:
        void readFilenameMask();
        void readAreInputToRemove();
        void readPathToOutput();
        void readOnFileRepeatAction();
        void readIsProcessingOnTimer();
        void readTimerPeriod();
        void readMaskToApply();

        std::istream* inputStream = nullptr;
        std::ostream* outputStream = nullptr;
        std::ostream* errorStream = nullptr;

        std::string filenameMask;
        bool areInputToRemove;
        QDir outputDirectory;
        OnFileRepeatAction onFileRepeatAction;
        bool isProcessingOnTimer;
        uint32_t timerPeriodMS;
        uint8_t maskToApply;

        bool errorIsOccurred = false;
        friend class Application;
    };

    void execute();
    bool debugIsEnabled() const;

private:
    void readUserSettings();
    void runProcess();

    void processFilename(const QString& filename) const;
    void prepareOutputFilename(QString& filename) const;
    void manageCreateNewFileWithCounterInFilename(QString& filename) const;

    void collectWorkingDirectory();
    void collectOutputDirectory();

    std::istream* inputStream = nullptr;
    std::ostream* outputStream = nullptr;
    std::ostream* errorStream = nullptr;
    QCoreApplication* qCoreApplication = nullptr;
    bool debugIsEnabledValue = false;

    UserSettings* userSettings = nullptr;

    QDir workingDirectory;

    QVector<QString> workingFilenames;
    QVector<QString> outputFilenames;
};

#endif // APPLICATION_H
