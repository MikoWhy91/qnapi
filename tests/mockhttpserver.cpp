#include "mockhttpserver.h"

#include <QHostAddress>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QUrl>

namespace {

QByteArray reasonPhrase(int status) {
  switch (status) {
    case 200:
      return "OK";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 406:
      return "Not Acceptable";
    case 429:
      return "Too Many Requests";
    case 500:
      return "Internal Server Error";
    case 503:
      return "Service Unavailable";
    default:
      return "Status";
  }
}

// returns true and fills request once buffer holds a complete request
bool parseRequest(QByteArray& buffer, MockHttpServer::Request& request) {
  int headerEnd = buffer.indexOf("\r\n\r\n");
  if (headerEnd < 0) return false;

  QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
  QList<QByteArray> requestLine = lines.takeFirst().trimmed().split(' ');
  if (requestLine.size() < 2) return false;

  MockHttpServer::Request r;
  r.method = requestLine[0];
  QUrl url(QString::fromLatin1(requestLine[1]));
  r.path = url.path();
  r.rawQuery = url.query(QUrl::FullyEncoded).toLatin1();
  r.query = QUrlQuery(url);
  for (const QByteArray& line : lines) {
    int colon = line.indexOf(':');
    if (colon < 0) continue;
    r.headers.insert(line.left(colon).trimmed().toLower(),
                     line.mid(colon + 1).trimmed());
  }

  int contentLength = r.headers.value("content-length").toInt();
  if (buffer.size() < headerEnd + 4 + contentLength) return false;
  r.body = buffer.mid(headerEnd + 4, contentLength);
  buffer.remove(0, headerEnd + 4 + contentLength);
  request = r;
  return true;
}

}  // namespace

MockHttpServer::MockHttpServer(QObject* parent) : QObject(parent) {
  connect(&server, &QTcpServer::newConnection, this,
          &MockHttpServer::onNewConnection);
  handler = [](const Request&) { return json(404, "{}"); };
}

bool MockHttpServer::listen() {
  return server.listen(QHostAddress::LocalHost, 0);
}

QString MockHttpServer::baseUrl() const {
  return QString("http://127.0.0.1:%1").arg(server.serverPort());
}

void MockHttpServer::setHandler(Handler h) { handler = std::move(h); }

void MockHttpServer::onNewConnection() {
  while (QTcpSocket* socket = server.nextPendingConnection()) {
    QSharedPointer<QByteArray> buffer(new QByteArray);
    connect(socket, &QTcpSocket::disconnected, socket,
            &QTcpSocket::deleteLater);
    connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer]() {
      buffer->append(socket->readAll());
      Request request;
      while (parseRequest(*buffer, request)) {
        received << request;
        Response response = handler(request);
        if (response.hang) return;
        QByteArray reply = "HTTP/1.1 " + QByteArray::number(response.status) +
                           " " + reasonPhrase(response.status) + "\r\n";
        reply += "Content-Type: " + response.contentType + "\r\n";
        reply += "Content-Length: " + QByteArray::number(response.body.size()) +
                 "\r\n";
        reply += "Connection: close\r\n\r\n";
        reply += response.body;
        socket->write(reply);
        socket->disconnectFromHost();
        return;
      }
    });
  }
}
