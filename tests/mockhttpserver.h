#ifndef QNAPI_TESTS_MOCKHTTPSERVER_H
#define QNAPI_TESTS_MOCKHTTPSERVER_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QUrlQuery>
#include <functional>

// Minimal HTTP/1.1 server on 127.0.0.1 for tests. It runs in the test's
// thread, which works because the code under test spins its own event loop
// while it waits for replies.
class MockHttpServer : public QObject {
  Q_OBJECT

 public:
  struct Request {
    QByteArray method;
    QString path;
    QUrlQuery query;
    QByteArray rawQuery;
    QMap<QByteArray, QByteArray> headers;  // lower-case names
    QByteArray body;

    QByteArray header(const QByteArray& name) const {
      return headers.value(name.toLower());
    }
  };

  struct Response {
    int status = 200;
    QByteArray body;
    QByteArray contentType = "application/json";
    // the connection is kept open without an answer, to test timeouts
    bool hang = false;
  };

  using Handler = std::function<Response(const Request&)>;

  explicit MockHttpServer(QObject* parent = nullptr);

  bool listen();
  QString baseUrl() const;

  void setHandler(Handler handler);
  QList<Request> requests() const { return received; }
  void clearRequests() { received.clear(); }

  static Response json(int status, const QByteArray& body) {
    Response r;
    r.status = status;
    r.body = body;
    return r;
  }

 private slots:
  void onNewConnection();

 private:
  QTcpServer server;
  Handler handler;
  QList<Request> received;
};

#endif  // QNAPI_TESTS_MOCKHTTPSERVER_H
