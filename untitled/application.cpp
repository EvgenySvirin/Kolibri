#include "application.h"
#include <QThread>

Application::Application(std::istream* inputStream,
                         std::ostream* outputStream,
                         std::ostream* errorStream,
                         QCoreApplication* qCoreApplication,
                         const bool& debugIsEnabledValue):
  inputStream(inputStream), outputStream(outputStream),
  errorStream(errorStream), qCoreApplication(qCoreApplication),
  debugIsEnabledValue(debugIsEnabledValue) {}

void Application::execute() {
    readUserSettings();
    if (userSettings->errorIsOccurred) {
        *outputStream << "Some errors while creating user settings have occurred, do you still wish to continue, 0 - No, 1 - Yes";

        int answer;
        *inputStream >> answer;
        if (!answer) {
            return;
        }
    }
    runProcess();

    if (!userSettings->isProcessingOnTimer) {
        return;
    }
    while (true) {
        QThread::msleep(userSettings->timerPeriodMS);
        runProcess();
    }
}

bool Application::debugIsEnabled() const {
   return debugIsEnabledValue;
}

void Application::readUserSettings() {
    workingDirectory = QDir(qCoreApplication->applicationDirPath());

    userSettings = new UserSettings(inputStream, outputStream, errorStream);
    userSettings->readUserSettings();
}

Application::UserSettings::UserSettings(std::istream* inputStream,
                                        std::ostream* outputStream,
                                        std::ostream* errorStream):
                 inputStream(inputStream), outputStream(outputStream),
                 errorStream(errorStream) {}

void Application::UserSettings::readUserSettings() {
    readFilenameMask();
    readAreInputToRemove();
    readPathToOutput();
    readOnFileRepeatAction();
    readIsProcessingOnTimer();
    readTimerPeriod();
    readMaskToApply();
}

void Application::UserSettings::readFilenameMask() {
    (*outputStream) << "Input filename mask:";
    *inputStream >> filenameMask;
}

void Application::UserSettings::readAreInputToRemove() {
    *outputStream << "Input 1 if input files are to be removed after processed else input 0:";
    *inputStream >> areInputToRemove;
}

void Application::UserSettings::readPathToOutput() {
    *outputStream << "Input path of a directory where results are to be saved:";
    std::string s;
    *inputStream >> s;

    outputDirectory = QDir(QString::fromStdString(s));
    if (!outputDirectory.exists()) {
        *errorStream << "\nDirectory" << s << " not accessible or does not exist, better to rerun program\n";
        errorIsOccurred = true;
    }
}

void Application::UserSettings::readOnFileRepeatAction() {
    *outputStream << "Input type of action to do if a file to save is already exists,"
                     " 0 overwrirte, 1 - saving with another filename with appended counter:";
    size_t n;
    *inputStream >> n;
    switch(n) {
    case 0:
        onFileRepeatAction = Application::OnFileRepeatAction::overwrite;
        break;
    case 1:
        onFileRepeatAction = Application::OnFileRepeatAction::createNewFileWithCounterInFilename;
        break;
    default:
        errorIsOccurred = true;
        *errorStream << "\nIncorrect file repeat action type input, better to rerun program\n";
        break;
    }
}

void Application::UserSettings::readIsProcessingOnTimer() {
    *outputStream << "Input 1 if application processes every chosen time period else if processes just once input 0:";
    *inputStream >> isProcessingOnTimer;
}

void Application::UserSettings::readTimerPeriod() {
    if (!isProcessingOnTimer) {
        return;
    }
    *outputStream << "Input time period for process to repeat in milliseconds:";
    *inputStream >> timerPeriodMS;
}

void Application::UserSettings::readMaskToApply() {
    *outputStream << "Input mask in binary representation, not bigger than a byte, to apply to each file byte, example 4 = 100:";
    std::string s;
    *inputStream >> s;

    const size_t base = 2;
    bool isCorrect = false;
    maskToApply = QString::fromStdString(s).toInt(&isCorrect, base);
    if (!isCorrect) {
        *errorStream << "\nWrong input of mask to apply, better to rerun program\n";
        errorIsOccurred = true;
    }
}

void Application::runProcess() {
    collectWorkingDirectory();
    collectOutputDirectory();
    for (const auto& filename : workingFilenames) {
        processFilename(filename);

        if (debugIsEnabled()) {
            *outputStream << filename.toStdString() << " processed\n";
        }
    }
    if (debugIsEnabled()) {
        *outputStream << "One whole process completed\n";
    }
}

void Application::processFilename(const QString &filename) const {
    QFile fileSrc(filename);
    if (fileSrc.isOpen()) {
        *outputStream << fileSrc.fileName().toStdString() << "is already opened";
        return;
    }
    auto outputFilename = filename;
    prepareOutputFilename(outputFilename);
    QFile fileDst(userSettings->outputDirectory.absoluteFilePath(outputFilename));

    fileSrc.open(QIODevice::ReadOnly);
    fileDst.open(QIODevice::WriteOnly);

    const size_t bufferSize = 128;
    auto bytes = fileSrc.read(bufferSize);
    while (bytes.size() != 0) {
        for (auto& byte : bytes) {
            byte ^= userSettings->maskToApply;
        }
        fileDst.write(bytes, bytes.size());
        bytes = fileSrc.read(bufferSize);
    }
    if (userSettings->areInputToRemove) {
        fileSrc.remove();
    }
}

void Application::prepareOutputFilename(QString& filename) const {
    switch (userSettings->onFileRepeatAction) {
    case Application::OnFileRepeatAction::overwrite:
        return;
    case Application::OnFileRepeatAction::createNewFileWithCounterInFilename:
        manageCreateNewFileWithCounterInFilename(filename);
        break;
    default:
        *errorStream << "\nIncorrect file repeat action type input occurred while processing files \n";
        break;
    }
}

void Application::manageCreateNewFileWithCounterInFilename(QString& filename) const {
    if (!outputFilenames.contains(filename)) {
        return;
    }

    auto filenameSplitted = filename.split(QString("."));

    int indexOfNumber = 1 < filenameSplitted.size() ? filenameSplitted.size() - 2 : 0;
    auto& partWithNumber = filenameSplitted[indexOfNumber];
    QString numberInString("");
    int i = partWithNumber.size() - 1;
    for (; 0 <= i; --i) {
        if (partWithNumber[i].isDigit()) {
            numberInString.append(partWithNumber[i]);
        } else {
            break;
        }
    }

    std::reverse(numberInString.begin(), numberInString.end());
    if (numberInString.size() == 0) {
        numberInString = "0";
    }
    numberInString = QString::number(numberInString.toInt() + 1);

    partWithNumber = partWithNumber.mid(0, i + 1) + numberInString;
    filename = filenameSplitted.join(".");

    manageCreateNewFileWithCounterInFilename(filename);
}

void Application::collectWorkingDirectory() {
    workingFilenames = workingDirectory.entryList(
                QStringList()
                << (QString::fromStdString("*" + userSettings->filenameMask)), QDir::Files).toVector();
}

void Application::collectOutputDirectory() {
    outputFilenames = userSettings->outputDirectory
            .entryList(QStringList(), QDir::Files).toVector();
}
