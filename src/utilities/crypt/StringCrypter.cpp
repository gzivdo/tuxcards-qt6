/***************************************************************************
                          StringCrypter.cpp  -  description
                             -------------------
    begin                : Fri Jul 18 2003
    copyright            : (C) 2003 by Alexander Theel
    email                : alex.theel@gmx.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "BlowFish.h"
#include "MD5.h"

#include <QString>
#include <QByteArray>
#include <QIODevice>
#include <iostream>
#include <cstring>

#include "StringCrypter.h"

const int StringCrypter::BUFFER_SIZE = 512;

const char* TUX_ENCRYPT_HEADER = "Fh_enc:BF10";

const int BFISH_BUF_MOD = 8;

void StringCrypter::encryptString( const QString& sInputString, const QString& sPassWd,
                                   QByteArray& encryptedData )
{
   if ( sPassWd.isNull() || sPassWd == "" )
   {
      std::cerr<<"TuxCards ERROR: StringCrypter::encryptString: No passwd given for: "
              << sInputString.toStdString()  << std::endl;
      return;
   }

   QByteArray passBytes = sPassWd.toUtf8();

   CMD5 md5;
   md5.MD5Init();
   md5.MD5Update((unsigned char *)passBytes.data(), passBytes.size());
   md5.MD5Final();

   CBlowFish fish;
   fish.Initialize( (unsigned char*)passBytes.data(), passBytes.size() );

   encryptedData.clear();

   encryptedData.append( TUX_ENCRYPT_HEADER, (int)strlen(TUX_ENCRYPT_HEADER) );

   unsigned char passWordHashValue[16];
   memset( passWordHashValue, 0, 16 );
   memcpy( passWordHashValue, md5.GetDigestBinary(), 16);
   encryptedData.append( (const char*)passWordHashValue, 16 );

   union aInt iSize;
   iSize.theInt = sInputString.length();
   encryptedData.append( (char)iSize.i.byte0 );
   encryptedData.append( (char)iSize.i.byte1 );
   encryptedData.append( (char)iSize.i.byte2 );
   encryptedData.append( (char)iSize.i.byte3 );
   encryptedData.append( '*' );

   //NOTE: important => buffer must be multiple of 8 bytes (block cypher)
   unsigned char buf_inout[BUFFER_SIZE];

   QByteArray inputBytes = sInputString.toUtf8();
   int totalLen = inputBytes.size();
   int offset = 0;

   while ( offset < totalLen )
   {
      int partLen = std::min((int)BUFFER_SIZE, totalLen - offset);
      memset( buf_inout, 0, BUFFER_SIZE );
      memcpy( buf_inout, inputBytes.constData() + offset, partLen );

      int encryptPadding = 0;
      if ( partLen % BFISH_BUF_MOD != 0 )
         encryptPadding = BFISH_BUF_MOD - partLen % BFISH_BUF_MOD;

      int iEncodedSize = fish.Encode( buf_inout, buf_inout, partLen + encryptPadding );
      encryptedData.append( (const char*)buf_inout, iEncodedSize );

      offset += BUFFER_SIZE;
   }
}

int StringCrypter::decryptString( const QByteArray& encryptedData,
                                  const QString& sPassWd, QString& sOutputString )
{
   unsigned char buf_inout[BUFFER_SIZE];
   CBlowFish fish;

   if (encryptedData.size() <= 0)
      return ERROR_INVALID_ENCRYPTEDDATA;

   char sHeader[15];
   memset( sHeader, 0, sizeof(sHeader) );
   for ( int i = 0; i < (int)strlen(TUX_ENCRYPT_HEADER); i++ )
   {
      sHeader[i] = encryptedData[i];
   }

   if ( 0 != strncmp(sHeader, TUX_ENCRYPT_HEADER, strlen(TUX_ENCRYPT_HEADER)) )
   {
      std::cout<<"Invalid file header (not encrypted with TuxCards)!"<<std::endl;
      return ERROR_INVALID_FILEHEADER;
   }

   unsigned char sHash[16];
   memset( sHash, 0, 16 );
   int offset = (int)strlen(TUX_ENCRYPT_HEADER);
   for ( int i = 0; i < 16; i++ )
   {
      sHash[i] = encryptedData[offset + i];
   }

   QByteArray passBytes = sPassWd.toUtf8();

   CMD5 md5;
   md5.MD5Init();
   md5.MD5Update( (unsigned char *)passBytes.data(), passBytes.size() );
   md5.MD5Final();

   if ( 0 != memcmp(sHash, md5.GetDigestBinary(), 16) )
   {
      return ERROR_INVALID_PASSWD;
   }

   union aInt iSize;
   iSize.i.byte0 = encryptedData[offset+16+0];
   iSize.i.byte1 = encryptedData[offset+16+1];
   iSize.i.byte2 = encryptedData[offset+16+2];
   iSize.i.byte3 = encryptedData[offset+16+3];


   int iIndexOfAsterisc = offset+16+3+1;
   int tmpArraySize = encryptedData.size() - iIndexOfAsterisc - 1;
   QByteArray tmpArray;
   tmpArray.resize(tmpArraySize);
   for ( int i = iIndexOfAsterisc+1; i < encryptedData.size(); i++ )
   {
      tmpArray[i-iIndexOfAsterisc-1] = encryptedData[i];
   }

   fish.Initialize( (unsigned char *)passBytes.data(), passBytes.size() );

   QByteArray leftArray;
   int bufSize;

   bufSize = StringCrypter::min(tmpArray.size(), BUFFER_SIZE);
   sOutputString = "";

   while ( bufSize > 0 )
   {
      getLeftBytes(leftArray, tmpArray, bufSize );
      memset( buf_inout, 0, BUFFER_SIZE );
      memcpy( buf_inout, leftArray.data(), bufSize);

      int decryptPadding = 0;
      if ( bufSize % BFISH_BUF_MOD != 0 )
         decryptPadding = BFISH_BUF_MOD - bufSize % BFISH_BUF_MOD;

      fish.Decode( buf_inout, buf_inout, bufSize + decryptPadding );

      if ( bufSize < BUFFER_SIZE )
         sOutputString += QString::fromUtf8( (const char*) buf_inout, -1 );
      else
         sOutputString += QString::fromUtf8( (const char*) buf_inout, bufSize );

      bufSize = StringCrypter::min(tmpArray.size(), BUFFER_SIZE);
   }

   return NO_ERROR;
}

int StringCrypter::min(int num1, int num2)
{
    if (num1 < num2)
        return num1;
    else
        return num2;
}

void StringCrypter::getLeftBytes( QByteArray& dest, QByteArray& source, uint len )
{
   dest.clear();

   if ( (uint)source.size() < len )
      len = source.size();

   dest.resize(len);
   for ( uint i = 0; i < len; i++ )
   {
      dest[i] = source[i];
   }

   int newSize = source.size() - len;
   QByteArray tmp;
   tmp.resize(newSize);
   for ( int i = 0; i < newSize; i++ )
   {
      tmp[i] = source[i + len];
   }

   source = tmp;
}
