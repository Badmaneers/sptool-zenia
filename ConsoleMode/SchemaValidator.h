#ifndef SCHEMAVALIDATOR_H
#define SCHEMAVALIDATOR_H

#include <QString>
#include <QFile>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include "../Err/Exception.h"

namespace ConsoleMode
{
class MessageHandler;
class SchemaValidator
{
public:
    SchemaValidator(const QString& _schema_file);
    ~SchemaValidator();

    void Validate(const QString& xml_file);
    void LoadFile(const QString &filename);

private:
    SchemaValidator(const SchemaValidator &);
    SchemaValidator & operator=(const SchemaValidator &);

private:
    QString schema_file;
    xercesc::XercesDOMParser *schema_parser;
    MessageHandler *msg_handler;
};

}

#endif // SCHEMAVALIDATOR_H