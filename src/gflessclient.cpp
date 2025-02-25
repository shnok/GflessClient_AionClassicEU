#include "gflessclient.h"
#include "injector.h"

GflessClient::GflessClient(QObject *parent) : QObject(parent)
{
    pipe = nullptr;
    gfServer = new QLocalServer(this);
    gfServer->listen("GameforgeClientJSONRPC");
    token = "";
    displayName = "";

    connect(gfServer, &QLocalServer::newConnection, this, &GflessClient::handleNewConnection);
}

bool GflessClient::openClient(const QString& displayName, const QString& token, const QString &gameClientPath, const int &gameLanguage, bool autoLogin, const QString& gsid)
{
    this->displayName = displayName;
    this->token = token;

    int index = gameClientPath.lastIndexOf("/aionclassic.bin");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    QProcess process;
    QProcessEnvironment env = process.processEnvironment();
    qint64 pid;

    env.insert("_TNT_CLIENT_APPLICATION_ID", "d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb");
    //env.insert("_TNT_SESSION_ID", "12345678-1234-1234-1234-123456789012");
	env.insert("_TNT_SESSION_ID", gsid);
	
    process.setProcessEnvironment(env);
    process.setWorkingDirectory(directory);
    process.setProgram(gameClientPath);
	
	QStringList arguments;
    arguments << "-lang:FRA" << "-usehqserver" << "-hqip:79.110.83.156" << "-hqport:13001" << "-np:79.110.83.118" << "-dwsm" << "-loginex" << "-pwd16" << "-ingamewebshop" << "-npsa" << "-probability" << "-nospamcheck" << "-dnpshop" << "-dranking" << "-st" << "-dpetition" << "-recserveridx:1";
	process.setArguments(arguments);
	
    if (!process.startDetached(&pid))
    {
        qDebug() << "Error creating process";
        return false;
    }

    return true;
}

bool GflessClient::openClientSettings(const QString &gameClientPath)
{
    int index = gameClientPath.lastIndexOf("/aionclassic.bin");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    QString settingsPath = directory + "/NtConfig.exe";

    return QProcess::startDetached(settingsPath, {}, directory);
}

void GflessClient::handleNewConnection()
{
    pipe = gfServer->nextPendingConnection();
    connect(pipe, &QLocalSocket::readyRead, this, &GflessClient::handlePipe);
}

void GflessClient::handlePipe()
{
    QByteArray output;
    QJsonObject json = QJsonDocument::fromJson(pipe->readAll()).object();
    QString method = json["method"].toString();

    qDebug() << QJsonDocument(json).toJson();

    if (method == "ClientLibrary.isClientRunning")
        output = prepareResponse(json, "true");

    else if  (method == "ClientLibrary.initSession")
        output = prepareResponse(json, json["params"].toObject()["sessionId"].toString().toUtf8());

    else if (method == "ClientLibrary.queryAuthorizationCode")
        output = prepareResponse(json, token.toUtf8());

    else if (method == "ClientLibrary.queryGameAccountName")
        output = prepareResponse(json, displayName.toUtf8());

    else if (method == "ClientLibrary.queryGameDisplayLocale")
        output = prepareResponse(json, "fr_FR");
	
    else if (method == "ClientLibrary.queryGameAccountNumericId")
        output = prepareResponse(json, "0");
	
    else
        return;

    pipe->write(output);
}

QByteArray GflessClient::prepareResponse(const QJsonObject &request, const QString &response)
{
    QJsonObject resp;

    resp["id"] = request["id"];
    resp["jsonrpc"] = request["jsonrpc"];
    resp["result"] = response;

    return QJsonDocument(resp).toJson();
}
