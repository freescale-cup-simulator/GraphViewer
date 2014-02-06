#ifndef LOGER_H
#define LOGER_H

#include <QFile>
#include <QDataStream>
#include <QVector>
#include <QString>
#include <exception>

#include "common.h"

class Logger
{

public:

    enum Mode{Closed,Read,Write};

    Logger();

    void setFileName(const QString &filename);

    bool beginWrite();
    quint64 endWrite();
    bool canWrite();

    bool beginRead();
    quint64 endRead();
    bool canRead();


    Mode mode() {return m_mode;}

    Logger & operator <<(DataSet & dataset);
    Logger & operator >> (DataSet & dataset);

    ~Logger();

    class CorruptedStructureException {};
private:

    QFile * m_file;
    QDataStream m_stream;
    Mode m_mode;
    quint64 m_written;
    void log(QString text);

};
#endif // LOGER_H
