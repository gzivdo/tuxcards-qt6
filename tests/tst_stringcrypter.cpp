#include <QtTest>
#include <QString>
#include <QByteArray>
#include <QFile>

#include "utilities/crypt/StringCrypter.h"


class TestStringCrypter : public QObject
{
   Q_OBJECT
private slots:
   void init() { StringCrypter::clearKeyCache(); }
   void roundtripAscii();
   void roundtripUtf8();
   void roundtripEmptyString();
   void wrongPasswordIsRejected();
   void tamperedCiphertextIsRejected();
   void emptyBufferGivesError();
   void magicHeaderMatchesWriteBackend();
   void bf10FixtureRecognizedByMagic();
#ifdef TUXCARDS_BACKEND_MONOCYPHER
   void xchachaRoundtripAscii();
   void xchachaRoundtripUtf8();
   void xchachaWrongPasswordRejected();
   void xchachaTamperedRejected();
#endif
#if defined(TUXCARDS_BACKEND_OPENSSL) && defined(TUXCARDS_BACKEND_MONOCYPHER)
   void crossBackendDispatchByMagic();
#endif
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


void TestStringCrypter::magicHeaderMatchesWriteBackend()
{
   QByteArray enc;
   StringCrypter::encryptString("x", "pwd", enc);
#if defined(TUXCARDS_WRITE_BACKEND_AESGCM)
   QVERIFY(enc.startsWith("Fh_enc:AES256GCM-PBKDF2"));
#elif defined(TUXCARDS_WRITE_BACKEND_XCHACHA)
   QVERIFY(enc.startsWith("Fh_enc:XChaCha20Poly1305-Argon2i-v1"));
#else
   QFAIL("No TUXCARDS_WRITE_BACKEND_* macro defined");
#endif
}


#ifdef TUXCARDS_BACKEND_MONOCYPHER
// Tests below exercise the monocypher backend directly via the private
// helpers — we use the public dispatcher and just assert that the magic
// matches when needed. Since the public encryptString writes whichever
// backend was selected, we have to read+write through the same
// build. For coverage of "force-write XChaCha", reconfigure with
// -DTUXCARDS_WRITE_BACKEND=monocypher.

void TestStringCrypter::xchachaRoundtripAscii()
{
#  if !defined(TUXCARDS_WRITE_BACKEND_XCHACHA)
   QSKIP("Build write-backend != monocypher; this test requires it.");
#  endif
   QString plaintext = "The quick brown fox jumps over the lazy dog.";
   QString pwd = "correcthorsebatterystaple";
   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);
   QVERIFY(enc.startsWith("Fh_enc:XChaCha20Poly1305-Argon2i-v1"));
   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, pwd, out), StringCrypter::NO_ERROR);
   QCOMPARE(out, plaintext);
}


void TestStringCrypter::xchachaRoundtripUtf8()
{
#  if !defined(TUXCARDS_WRITE_BACKEND_XCHACHA)
   QSKIP("Build write-backend != monocypher; this test requires it.");
#  endif
   QString plaintext = QString::fromUtf8(
         "Привет, мир! 你好世界 🎉 — multi-byte UTF-8 payload.");
   QString pwd = "пароль-секрет";
   QByteArray enc;
   StringCrypter::encryptString(plaintext, pwd, enc);
   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, pwd, out), StringCrypter::NO_ERROR);
   QCOMPARE(out, plaintext);
}


void TestStringCrypter::xchachaWrongPasswordRejected()
{
#  if !defined(TUXCARDS_WRITE_BACKEND_XCHACHA)
   QSKIP("Build write-backend != monocypher; this test requires it.");
#  endif
   QByteArray enc;
   StringCrypter::encryptString("secret", "right", enc);
   StringCrypter::clearKeyCache();
   QString out;
   QCOMPARE(StringCrypter::decryptString(enc, "wrong", out),
            StringCrypter::ERROR_INVALID_PASSWD);
}


void TestStringCrypter::xchachaTamperedRejected()
{
#  if !defined(TUXCARDS_WRITE_BACKEND_XCHACHA)
   QSKIP("Build write-backend != monocypher; this test requires it.");
#  endif
   QByteArray enc;
   StringCrypter::encryptString("secret message", "pwd", enc);
   QVERIFY(enc.size() > 100);
   enc[enc.size() - 3] = enc[enc.size() - 3] ^ 0x77;
   QString out;
   QVERIFY(StringCrypter::decryptString(enc, "pwd", out) != StringCrypter::NO_ERROR);
}
#endif  // TUXCARDS_BACKEND_MONOCYPHER


#if defined(TUXCARDS_BACKEND_OPENSSL) && defined(TUXCARDS_BACKEND_MONOCYPHER)
void TestStringCrypter::crossBackendDispatchByMagic()
{
   // The dispatcher must route a blob to the correct backend by its
   // magic header, regardless of which backend was chosen for writing.
   // We can only synthesize one of the two formats from the public
   // API (whichever is the write backend), but we can at least verify
   // that magic-based routing works end-to-end for the write path.
   QByteArray enc;
   StringCrypter::encryptString("hello", "pwd", enc);
#  if defined(TUXCARDS_WRITE_BACKEND_AESGCM)
   QVERIFY(enc.startsWith("Fh_enc:AES256GCM-PBKDF2"));
#  else
   QVERIFY(enc.startsWith("Fh_enc:XChaCha20Poly1305-Argon2i-v1"));
#  endif

   // Corrupt the magic to a recognized-but-other one; dispatcher should
   // return a non-NO_ERROR code (either passwd reject from the other
   // backend, or invalid-data).
   QByteArray corrupted = enc;
   // Replace first chars with the OTHER magic, padded with zeros if shorter.
#  if defined(TUXCARDS_WRITE_BACKEND_AESGCM)
   const char* otherMagic = "Fh_enc:XChaCha20Poly1305-Argon2i-v1";
#  else
   const char* otherMagic = "Fh_enc:AES256GCM-PBKDF2";
#  endif
   const int otherLen = (int)qstrlen(otherMagic);
   if ( corrupted.size() >= otherLen ) {
      memcpy(corrupted.data(), otherMagic, otherLen);
      QString out;
      int rc = StringCrypter::decryptString(corrupted, "pwd", out);
      QVERIFY(rc != StringCrypter::NO_ERROR);
   }
}
#endif


// The fixture under tests/test-files/1-encrypted.txt is a base64-encoded
// BF10 blob from the original TuxCards 2010 test driver. Its password
// was lost long before this fork, so we can't decrypt it — but we can
// still verify that identifyBlobFormat() correctly tags it as BF10
// after base64 decode. That alone proves the legacy magic detection
// still works on a real-world 2010-era blob, which is the part the
// production read path depends on.
void TestStringCrypter::bf10FixtureRecognizedByMagic()
{
   const QString path = QStringLiteral(TUXCARDS_TEST_FILES_DIR)
                        + "/1-encrypted.txt";
   QFile f(path);
   QVERIFY2(f.open(QIODevice::ReadOnly),
            qPrintable("can't open fixture: " + path));
   const QByteArray enc = QByteArray::fromBase64(f.readAll());
   QVERIFY(!enc.isEmpty());
   QCOMPARE(StringCrypter::identifyBlobFormat(enc),
            (int)StringCrypter::BLOB_LEGACY_BF);
}


QTEST_GUILESS_MAIN(TestStringCrypter)
#include "tst_stringcrypter.moc"
