#include <QtTest>

#include "Either.h"
#include "Maybe.h"

Q_DECLARE_METATYPE(int*)
Q_DECLARE_METATYPE(Maybe<int>)

// the parts of the vendored qt-maybe headers that QNapi relies on
class TestMaybe : public QObject {
  Q_OBJECT

 private slots:
  void justAndNothing() {
    Maybe<int> ok = just(42);
    Maybe<int> zero = just(0);
    Maybe<int> none = nothing();
    QVERIFY(ok);
    QCOMPARE(ok.value(), 42);
    QVERIFY(zero);
    QCOMPARE(zero.value(), 0);
    QVERIFY(!none);

    Maybe<QString> str = just(QString("text"));
    QCOMPARE(str.value(), QString("text"));
    Maybe<QString> fromCString = just("hello");
    QCOMPARE(fromCString.value(), QString("hello"));
  }

  void nullPointerIsNothing() {
    int value = 1;
    int* null = nullptr;
    Maybe<int*> fromNull = just(null);
    Maybe<int*> fromValue = just(&value);
    QVERIFY(!fromNull);
    QVERIFY(fromValue);
    QCOMPARE(fromValue.value(), &value);
  }

  void either() {
    Either<QString, int> text = some(QString("error"));
    Either<QString, int> number = some(7);
    QVERIFY(text.is<QString>());
    QVERIFY(!text.is<int>());
    QCOMPARE(text.as<QString>(), QString("error"));
    QVERIFY(number.is2nd());
    QCOMPARE(number.as2nd(), 7);
  }

  void eitherOfMaybe() {
    Maybe<int> inner = just(3);
    Either<QString, Maybe<int>> e = some(inner);
    QVERIFY(e.is<Maybe<int>>());
    QVERIFY(e.as<Maybe<int>>());
    QCOMPARE(e.as<Maybe<int>>().value(), 3);

    Maybe<int> empty = nothing();
    Either<QString, Maybe<int>> e2 = some(empty);
    QVERIFY(e2.is<Maybe<int>>());
    QVERIFY(!e2.as<Maybe<int>>());
  }
};

QTEST_GUILESS_MAIN(TestMaybe)
#include "tst_maybe.moc"
