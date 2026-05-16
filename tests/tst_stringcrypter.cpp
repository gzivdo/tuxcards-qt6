#include <QtTest>
#include <QString>
#include <QByteArray>

#include "utilities/crypt/StringCrypter.h"


class TestStringCrypter : public QObject
{
   Q_OBJECT
private slots:
   void roundtripAscii();
   void roundtripUtf8();
   void roundtripEmptyString();
   void wrongPasswordIsRejected();
   void tamperedCiphertextIsRejected();
   void emptyBufferGivesError();
   void magicHeaderIsAESGCM();
};


void TestStringCrypter::roundtripAscii()
{
   QString plaintext = "The quick brown fox jumps over the lazy dog.";
   QString pwd = "correcthorsebatterystaple";

   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);
   QVERIFY(!enc.isEmpty());

   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, pwd, out), StringCrypter::NO_ERROR);
   QCOMPARE(out, plaintext);
}


void TestStringCrypter::roundtripUtf8()
{
   QString plaintext = QString::fromUtf8(
         "Привет, мир! 你好世界 🎉 — multi-byte UTF-8 payload.");
   QString pwd = "пароль-секрет";

   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);

   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, pwd, out), StringCrypter::NO_ERROR);
   QCOMPARE(out, plaintext);
}


void TestStringCrypter::roundtripEmptyString()
{
   QString plaintext = "";
   QString pwd = "x";

   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);
   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, pwd, out), StringCrypter::NO_ERROR);
   QCOMPARE(out, plaintext);
}


void TestStringCrypter::wrongPasswordIsRejected()
{
   QString plaintext = "secret message";
   QString pwd = "right";

   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);

   QString out;
   int rc = StringCrypter::decryptString(enc, "wrong", out);
   QCOMPARE(rc, StringCrypter::ERROR_INVALID_PASSWD);
   QVERIFY(out.isEmpty());
}


void TestStringCrypter::tamperedCiphertextIsRejected()
{
   QString plaintext = "secret message";
   QString pwd = "right";

   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);

   // Flip a byte deep into the ciphertext section (well past the header).
   QVERIFY(enc.size() > 80);
   enc[enc.size() - 5] = enc[enc.size() - 5] ^ 0x55;

   QString out;
   int rc = StringCrypter::decryptString(enc, pwd, out);
   QVERIFY(rc != StringCrypter::NO_ERROR);
}


void TestStringCrypter::emptyBufferGivesError()
{
   QByteArray enc;
   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, "pwd", out),
            StringCrypter::ERROR_INVALID_ENCRYPTEDDATA);
}


void TestStringCrypter::magicHeaderIsAESGCM()
{
   QByteArray enc;
   StringCrypter::encryptString("x", "pwd", enc);
   QVERIFY(enc.startsWith("Fh_enc:AES256GCM-PBKDF2"));
}


QTEST_GUILESS_MAIN(TestStringCrypter)
#include "tst_stringcrypter.moc"
