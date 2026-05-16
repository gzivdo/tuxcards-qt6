#include <QtTest>
#include <QStringList>

#include "information/Path.h"


class TestPath : public QObject
{
   Q_OBJECT
private slots:
   void roundtripString();
   void emptyPath();
   void singleSegment();
};


void TestPath::roundtripString()
{
   Path p("Root//Folder//Note");      // legacy separator "//"
   QStringList list = p.getPathList();
   QCOMPARE(list.size(), 3);
   QCOMPARE(list[0], QString("Root"));
   QCOMPARE(list[1], QString("Folder"));
   QCOMPARE(list[2], QString("Note"));
   // toString() must round-trip through the same separator.
   QCOMPARE(Path(p.toString()).getPathList(), list);
}


void TestPath::emptyPath()
{
   Path p(QString(""));
   QCOMPARE(p.getPathList().size(), 0);
}


void TestPath::singleSegment()
{
   Path p("Standalone");
   QStringList list = p.getPathList();
   QCOMPARE(list.size(), 1);
   QCOMPARE(list[0], QString("Standalone"));
}


QTEST_GUILESS_MAIN(TestPath)
#include "tst_path.moc"
