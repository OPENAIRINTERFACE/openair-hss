/*
* Copyright (c) 2017 Sprint
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <iostream>
#include <unistd.h>

#include "sfile.h"

SFile::SFile()
{
   close();
}

SFile::~SFile()
{
}

void SFile::close()
{
   if ( m_stream.is_open() )
      m_stream.close();

   m_filename.clear();
   m_data.clear();
   m_datarn = 0;
   m_dataofs = 0;
   m_nextrn = 1;
   m_nextofs = 0;
}

bool SFile::open( const char *filename )
{
   close();

   m_filename = filename;

   m_stream.open( m_filename.c_str(), std::ifstream::in );

   return m_stream.is_open();
}

bool SFile::seek( uint32_t recnbr, std::ios::streampos offset )
{
   m_stream.clear();
   std::ios::streampos old = m_stream.tellg();

   m_stream.seekg( offset );

   if ( m_stream.good() )
   {
      m_nextrn = recnbr;
      m_nextofs = offset;
      return true;
   }
else { std::cout << "m_stream is not good after seekg, good=" << m_stream.good() << " eof=" << m_stream.eof() << " fail=" << m_stream.fail() << " bad=" << m_stream.bad() << std::endl; }
   
   m_stream.clear();
   m_stream.seekg( old );

   return false;
}

bool SFile::read()
{
   // create temporary buffer
   char data[2048];

   // seek to the beginning of the record
   m_stream.clear();
   m_stream.seekg( m_nextofs );

   // read the next line
   m_stream.getline( data, sizeof(data), '\n' );
   if ( m_stream.good() )
   {
      // update the info
      m_data = data;
      m_dataofs = m_nextofs;
      m_datarn = m_nextrn;
      m_nextofs = m_stream.tellg();
      m_nextrn++;
      return true;
   }

   return false;
}

bool SFile::renamed()
{
   return access( m_filename.c_str(), F_OK ) != 0;
}
