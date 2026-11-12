#include <QSignalSpy>
#include <QTest>

#include "controller.h"
#include "server.h"

class FakePointer : public Pointer
{
public:
    void move(double dx, double dy) override { moves.append({dx, dy}); }
    void click(Button button) override { clicks.append(button); }
    void scroll(int steps) override { scrolls.append(steps); }

    QList<QPointF> moves;
    QList<Button> clicks;
    QList<int> scrolls;
};

class FakeNotifier : public Notifier
{
public:
    void notify(const char *message, Urgency) override { messages.append(QString::fromUtf8(message)); }

    QStringList messages;
};

class NumouseTests : public QObject
{
    Q_OBJECT

private slots:
    void togglesWithTheExactNotifications()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);

        controller.toggle();
        QVERIFY(controller.isActive());
        controller.toggle();
        QVERIFY(!controller.isActive());
        QCOMPARE(notifier.messages, QStringList({"Numpad Mouse Actived", "Numpad Mouse Deactivated"}));

        // Repeating a state does not notify again.
        controller.disable();
        QCOMPARE(notifier.messages.size(), 2);
    }

    void ignoresKeysWhileInactive()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);

        controller.press(6);
        controller.press(5);
        QTest::qWait(Controller::DoubleClickMs + 50);
        QVERIFY(pointer.moves.isEmpty());
        QVERIFY(pointer.clicks.isEmpty());
    }

    void keysPointTheRightWay_data()
    {
        QTest::addColumn<int>("key");
        QTest::addColumn<int>("sx");
        QTest::addColumn<int>("sy");
        QTest::newRow("1") << 1 << -1 << 1;
        QTest::newRow("2") << 2 << 0 << 1;
        QTest::newRow("3") << 3 << 1 << 1;
        QTest::newRow("4") << 4 << -1 << 0;
        QTest::newRow("6") << 6 << 1 << 0;
        QTest::newRow("7") << 7 << -1 << -1;
        QTest::newRow("8") << 8 << 0 << -1;
        QTest::newRow("9") << 9 << 1 << -1;
    }

    void keysPointTheRightWay()
    {
        QFETCH(int, key);
        QFETCH(int, sx);
        QFETCH(int, sy);
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(key);
        QCOMPARE(pointer.moves.size(), 1); // A tap moves at once.
        const QPointF first = pointer.moves.first();
        auto sign = [](double v) { return v > 1e-9 ? 1 : v < -1e-9 ? -1 : 0; };
        QCOMPARE(sign(first.x()), sx);
        QCOMPARE(sign(first.y()), sy);
        controller.release(key);
    }

    void heldKeysCombineAndStop()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(8);
        controller.press(6);
        QVERIFY(controller.direction().x() > 0.7 && controller.direction().y() < -0.7);

        // Opposite keys cancel out.
        controller.press(4);
        QVERIFY(qAbs(controller.direction().x()) < 1e-9);

        controller.release(8);
        controller.release(6);
        controller.release(4);
        const int count = pointer.moves.size();
        QTest::qWait(Controller::TickMs * 5);
        QCOMPARE(pointer.moves.size(), count);
    }

    void movesKeepGoingWhileHeld()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(6);
        QTest::qWait(100);
        QVERIFY(pointer.moves.size() > 3);
        controller.release(6);
    }

    void accelerates()
    {
        QCOMPARE(Controller::speedAt(0), Controller::MinSpeed);
        QVERIFY(Controller::speedAt(Controller::RampMs / 2) > Controller::MinSpeed);
        QCOMPARE(Controller::speedAt(Controller::RampMs), Controller::MaxSpeed);
        QCOMPARE(Controller::speedAt(Controller::RampMs * 10), Controller::MaxSpeed);
    }

    void disableStopsDrift()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(2);
        controller.disable();
        const int count = pointer.moves.size();
        QTest::qWait(Controller::TickMs * 5);
        QCOMPARE(pointer.moves.size(), count);
        QVERIFY(controller.direction().isNull());
    }

    void singleFiveIsLeftClick()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(5);
        controller.release(5);
        QVERIFY(pointer.clicks.isEmpty()); // Waits to rule out a double press.
        QTRY_COMPARE_WITH_TIMEOUT(pointer.clicks.size(), 1, Controller::DoubleClickMs * 3);
        QCOMPARE(pointer.clicks.first(), Pointer::Left);
    }

    void doubleFiveIsRightClick()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(5);
        controller.release(5);
        QTest::qWait(Controller::DoubleClickMs / 3);
        controller.press(5);
        controller.release(5);
        QCOMPARE(pointer.clicks, QList<Pointer::Button>({Pointer::Right}));
        QTest::qWait(Controller::DoubleClickMs + 50);
        QCOMPARE(pointer.clicks.size(), 1); // No stray left click afterwards.
    }

    void plusScrollsUpEnterScrollsDown()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(Controller::ScrollUpKey);
        controller.release(Controller::ScrollUpKey);
        controller.press(Controller::ScrollDownKey);
        controller.release(Controller::ScrollDownKey);
        QCOMPARE(pointer.scrolls, QList<int>({-1, 1}));
        QTest::qWait(Controller::ScrollDelayMs + 50);
        QCOMPARE(pointer.scrolls.size(), 2); // Taps do not repeat.
        QVERIFY(pointer.moves.isEmpty());
    }

    void heldScrollRepeatsUntilRelease()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(Controller::ScrollDownKey);
        QTest::qWait(Controller::ScrollDelayMs / 2);
        QCOMPARE(pointer.scrolls.size(), 1); // Still in the initial pause.
        QTRY_VERIFY_WITH_TIMEOUT(pointer.scrolls.size() >= 4, Controller::ScrollDelayMs + Controller::ScrollRepeatMs * 6);
        controller.release(Controller::ScrollDownKey);
        const int count = pointer.scrolls.size();
        QTest::qWait(Controller::ScrollRepeatMs * 3);
        QCOMPARE(pointer.scrolls.size(), count);
        for (int step : pointer.scrolls)
            QCOMPARE(step, 1);
    }

    void disableStopsScrolling()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);
        controller.enable();

        controller.press(Controller::ScrollUpKey);
        controller.disable();
        QTest::qWait(Controller::ScrollDelayMs + Controller::ScrollRepeatMs * 3);
        QCOMPARE(pointer.scrolls.size(), 1);
    }

    void serverParsesCommands()
    {
        FakePointer pointer;
        FakeNotifier notifier;
        Controller controller(&pointer, &notifier);

        QCOMPARE(Server::handle(&controller, "status"), QByteArray("inactive\n"));
        QCOMPARE(Server::handle(&controller, "toggle"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "press 6"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "release 6"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "press add"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "release add"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "press enter"), QByteArray("active\n"));
        QCOMPARE(Server::handle(&controller, "release enter"), QByteArray("active\n"));
        QCOMPARE(pointer.scrolls, QList<int>({-1, 1}));
        QVERIFY(Server::handle(&controller, "press 0").startsWith("error"));
        QVERIFY(Server::handle(&controller, "press x").startsWith("error"));
        QVERIFY(Server::handle(&controller, "jump").startsWith("error"));
        QCOMPARE(Server::handle(&controller, "disable"), QByteArray("inactive\n"));
    }
};

QTEST_GUILESS_MAIN(NumouseTests)
#include "numouse_tests.moc"
