#include "SchemaValidator.h"
#include <QtGlobal>

#include <QDir>

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/util/XMLString.hpp>

#include "../Logger/Log.h"
#include "../Err/FlashToolErrorCodeDef.h"
#include "../Host/Inc/RuntimeMemory.h"

namespace ConsoleMode
{

class MessageHandler : public xercesc::ErrorHandler
{
public:
    MessageHandler()
        : m_line(-1),
          m_column(-1)
    {
    }

    QString statusMessage() const
    {
        return m_description;
    }

    QString line() const
    {
        return QString::number(m_line);
    }

    QString column() const
    {
        return QString::number(m_column);
    }

    void clear()
    {
        m_description.clear();
        m_line = -1;
        m_column = -1;
    }

    virtual void warning(const xercesc::SAXParseException &exc)
    {
        capture(exc);
    }

    virtual void error(const xercesc::SAXParseException &exc)
    {
        capture(exc);
    }

    virtual void fatalError(const xercesc::SAXParseException &exc)
    {
        capture(exc);
    }

    virtual void resetErrors()
    {
    }

private:
    void capture(const xercesc::SAXParseException &exc)
    {
        if (m_description.isEmpty())
        {
            char *msg = xercesc::XMLString::transcode(exc.getMessage());
            m_description = msg ? QString::fromUtf8(msg) : QString();
            xercesc::XMLString::release(&msg);
            m_line = exc.getLineNumber();
            m_column = exc.getColumnNumber();
        }
    }

private:
    QString m_description;
    long m_line;
    long m_column;
};

SchemaValidator::SchemaValidator(const QString& _schema_file)
    : schema_file(_schema_file), schema_parser(NULL), msg_handler(new MessageHandler())
{
    static bool xerces_inited = false;
    if(!xerces_inited) {
        xercesc::XMLPlatformUtils::Initialize();
        xerces_inited = true;
    }

    schema_parser = new xercesc::XercesDOMParser();
    schema_parser->setValidationScheme(xercesc::XercesDOMParser::Val_Always);
    schema_parser->setDoNamespaces(true);
    schema_parser->setDoSchema(true);
    schema_parser->setValidationSchemaFullChecking(true);
    schema_parser->setErrorHandler(msg_handler);

    QFile file(QDir::toNativeSeparators(schema_file));
    if(file.exists()) {
        QByteArray schema_loc = QDir::toNativeSeparators(schema_file).toUtf8();
        schema_parser->setExternalNoNamespaceSchemaLocation(schema_loc.constData());
    } else {
        LOGI("schema file (%s) does NOT exsit!", schema_file.toLocal8Bit().data());
    }

    Q_ASSERT(file.exists());
}

SchemaValidator::~SchemaValidator()
{
    if(msg_handler!=NULL)
    {
        delete msg_handler;
        msg_handler = NULL;
    }
    if(schema_parser!=NULL)
    {
        delete schema_parser;
        schema_parser = NULL;
    }
}

void SchemaValidator::Validate(const QString& xml_file)
{
    LOG("validating XML schema...");
    msg_handler->clear();

    QFile file(xml_file);
    file.open(QFile::ReadOnly);

    QByteArray file_path = xml_file.toUtf8();
    try
    {
        schema_parser->parse(file_path.constData());
    }
    catch(const xercesc::XMLException &e)
    {
        char *msg = xercesc::XMLString::transcode(e.getMessage());
        LOGI("XML Schema validation exception: %s", msg ? msg : "unknown");
        xercesc::XMLString::release(&msg);
    }
    catch(const std::exception &)
    {
        LOGI("XML Schema validation exception: std::exception");
    }

    if(!msg_handler->statusMessage().isEmpty())
    {
        QString msg("XML Schema validation failed: ");
        msg.append(msg_handler->statusMessage());
        msg.append("(line:").append(msg_handler->line());
        msg.append(", column:").append(msg_handler->column()).append(")");

        LOGI(msg.toLocal8Bit().constData());
        THROW_APP_EXCEPTION(-1, msg.toStdString());
    }
    LOG("XML schema validation passed!");
}

}