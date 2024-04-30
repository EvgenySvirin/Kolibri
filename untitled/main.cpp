#include <QCoreApplication>
#include <iostream>

#include "application.h"

int main(int argc, char *argv[])
{
    QCoreApplication qCoreApplication(argc, argv);
    const bool debugIsEnabledValue = false;
    Application application(&std::cin, &std::cout, &std::cerr, &qCoreApplication, debugIsEnabledValue);

    application.execute();
}
