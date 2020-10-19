#include "systemmanager.h"

SystemManager::SystemManager()
{

}

SystemManager::~SystemManager()
{
    if (log->isOpen())
    {
        log->close();
    }
}

bool SystemManager::createLogFile()
{
    // Archivo de logs del sistema
    log = new QFile("Log.txt");

    if (!log->open(QIODevice::WriteOnly))
    {
        return false;
    }

    return true;
}

bool SystemManager::saveLogFile()
{
    if (log->isOpen())
    {
        log->close();
    }

    else
    {
        return false;
    }

    if (!log->open(QIODevice::WriteOnly))
    {
        return false;
    }

    return true;
}

void SystemManager::LOG(QString logData)
{
    if (log->isOpen())
    {
        QDate date = QDate::currentDate();
        QTime time = QTime::currentTime();

        log->write(QString::asprintf("%i-", date.year()).toLatin1());

        if (date.month() < 10)
        {
            log->write(QString::asprintf("0%i-", date.month()).toLatin1());
        }

        else
        {
            log->write(QString::asprintf("%i-", date.month()).toLatin1());
        }

        if (date.day() < 10)
        {
            log->write(QString::asprintf("0%i-", date.day()).toLatin1());
        }

        else
        {
            log->write(QString::asprintf("%i-", date.day()).toLatin1());
        }

        if (time.hour() < 10)
        {
            log->write(QString::asprintf("0%i-", time.hour()).toLatin1());
        }

        else
        {
            log->write(QString::asprintf("%i-", time.hour()).toLatin1());
        }

        if (time.minute() < 10)
        {
            log->write(QString::asprintf("0%i-", time.minute()).toLatin1());
        }

        else
        {
            log->write(QString::asprintf("%i-", time.minute()).toLatin1());
        }

        if (time.second() < 10)
        {
            log->write(QString::asprintf("0%i: ", time.second()).toLatin1());
        }

        else
        {
            log->write(QString::asprintf("%i: ", time.second()).toLatin1());
        }

        log->write(logData.toLatin1() + "\r\n\n");
    }
}
