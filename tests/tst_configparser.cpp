#include <QtTest>
#include <QTemporaryFile>

#include "utilities/iniparser/configparser.h"


class TestConfigParser : public QObject
{
   Q_OBJECT
private slots:
   void roundtripStrings();
   void roundtripInts();
   void groupsAreIsolated();
};


void TestConfigParser::roundtripStrings()
{
   QTemporaryFile tf;
   QVERIFY(tf.open());
   const QString path = tf.fileName();
   tf.close();

   {
      ConfigParser p(path, true);
      p.setGroup("General");
      p.changeEntry(QString("Name"), QString("Alice"));
      p.changeEntry(QString("City"), QString("Wonderland"));
   }

   ConfigParser p(path, false);
   p.setGroup("General");
   QCOMPARE(p.readEntry("Name", ""), QString("Alice"));
   QCOMPARE(p.readEntry("City", ""), QString("Wonderland"));
   QCOMPARE(p.readEntry("Missing", "fallback"), QString("fallback"));
}


void TestConfigParser::roundtripInts()
{
   QTemporaryFile tf;
   QVERIFY(tf.open());
   const QString path = tf.fileName();
   tf.close();

   {
      ConfigParser p(path, true);
      p.setGroup("Numbers");
      p.changeEntry(QString("Age"),   42);
      p.changeEntry(QString("Year"),  2026);
   }

   ConfigParser p(path, false);
   p.setGroup("Numbers");
   QCOMPARE(p.readNumEntry("Age", 0),   42);
   QCOMPARE(p.readNumEntry("Year", 0),  2026);
   QCOMPARE(p.readNumEntry("None", 7),  7);
}


void TestConfigParser::groupsAreIsolated()
{
   QTemporaryFile tf;
   QVERIFY(tf.open());
   const QString path = tf.fileName();
   tf.close();

   {
      ConfigParser p(path, true);
      p.setGroup("A");
      p.changeEntry(QString("Key"), QString("a-value"));
      p.setGroup("B");
      p.changeEntry(QString("Key"), QString("b-value"));
   }

   ConfigParser p(path, false);
   p.setGroup("A");
   QCOMPARE(p.readEntry("Key", ""), QString("a-value"));
   p.setGroup("B");
   QCOMPARE(p.readEntry("Key", ""), QString("b-value"));
}


QTEST_GUILESS_MAIN(TestConfigParser)
#include "tst_configparser.moc"
