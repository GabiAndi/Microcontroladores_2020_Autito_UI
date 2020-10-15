#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include <QFile>
#include <QDate>
#include <QTime>

class SystemManager
{
    public:
        SystemManager();
        ~SystemManager();

        bool createLogFile();

        void LOG(QString logData);

    private:
        QFile *log = nullptr;
};

#endif // SYSTEMMANAGER_H
