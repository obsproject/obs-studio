/* ****************************************************************************** *\

Copyright (C) 2013-2014 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfx_vector.h

\* ****************************************************************************** */

#pragma once
#include "mfxstructures.h"
#include <exception>

namespace MFX 
{
    template <class T>
    class iterator_tmpl 
    {
        template <class U> friend class MFXVector;
        mfxU32 mIndex;
        T* mRecords;
        iterator_tmpl(mfxU32 index , T * records) 
            : mIndex (index)
            , mRecords(records)
        {}
    public:
        iterator_tmpl() 
            : mIndex ()
            , mRecords() 
        {}
        bool  operator ==(const iterator_tmpl<T> & that )const 
        {
            return mIndex == that.mIndex;
        }
        bool  operator !=(const iterator_tmpl<T> & that )const 
        {
            return mIndex != that.mIndex;
        }
        mfxU32 operator - (const iterator_tmpl<T> &that) const 
        {
            return mIndex - that.mIndex;
        }
        iterator_tmpl<T> & operator ++() 
        {
            mIndex++;
            return * this;
        }
        iterator_tmpl<T> & operator ++(int) 
        {
            mIndex++;
            return * this;
        }
        T & operator *() 
        {
            return mRecords[mIndex];
        }
        T * operator ->() 
        {
            return mRecords + mIndex;
        }
    };

    class MFXVectorRangeError : public std::exception
    {
    };

    template <class T>
    class MFXVector  
    {
        T*      mRecords;
        mfxU32  mNrecords;
    public:
        MFXVector()
            : mRecords()
            , mNrecords()
        {}
        MFXVector(const MFXVector & rhs)
            : mRecords()
            , mNrecords()
        {
            insert(end(), rhs.begin(), rhs.end());
        }
        MFXVector & operator = (const MFXVector & rhs)
        {
            if (this != &rhs) 
            {
                clear();
                insert(end(), rhs.begin(), rhs.end());
            }
            return *this;
        }
        virtual ~MFXVector ()
        {
            clear();
        }
        typedef iterator_tmpl<T> iterator;

        iterator begin() const 
        {
            return iterator(0u, mRecords);
        }
        iterator end() const 
        {
            return iterator(mNrecords, mRecords);
        }
        void insert(iterator where, iterator beg_iter, iterator end_iter) 
        {
            mfxU32 elementsToInsert = (end_iter - beg_iter);
            if (!elementsToInsert)
            {
                return;
            }
            if (where.mIndex > mNrecords)
            {
                throw MFXVectorRangeError();
            }

            T *newRecords = new T[mNrecords + elementsToInsert]();
            mfxU32 i = 0;
            
            // save left
            for (; i < where.mIndex; i++) 
            {
                newRecords[i] = mRecords[i];
            }
            // insert
            for (; beg_iter != end_iter; beg_iter++, i++) 
            {
                newRecords[i] = *beg_iter;
            }
    
            //save right
            for (; i < mNrecords + elementsToInsert; i++) 
            {
                newRecords[i] = mRecords[i - elementsToInsert];
            }

            delete [] mRecords;

            mRecords = newRecords;
            mNrecords = i;
        }
        T& operator [] (mfxU32 idx) 
        {
          return mRecords[idx];
        }
        void push_back(const T& obj) 
        {
            T *newRecords = new T[mNrecords + 1]();
            mfxU32 i = 0;
            for (; i <mNrecords; i++) 
            {
                newRecords[i] = mRecords[i];
            }
            newRecords[i] = obj;
            delete [] mRecords;

            mRecords = newRecords;
            mNrecords = i + 1;
            
        }
        void erase (iterator at) 
        {
            if (at.mIndex >= mNrecords)
            {
                throw MFXVectorRangeError();
            }
            mNrecords--; 
            mfxU32 i = at.mIndex;
            for (; i != mNrecords; i++)
            {
                mRecords[i] = mRecords[i+1];
            }
            //destroy last element
            mRecords[i] = T();
        }
        void resize(mfxU32 nSize) 
        {
            T * newRecords = new T[nSize]();
            for (mfxU32 i = 0; i <mNrecords; i++) 
            {
                newRecords[i] = mRecords[i];
            }
            delete [] mRecords;
            mRecords = newRecords;
            mNrecords = nSize;
        }
        mfxU32 size() const 
        {
            return mNrecords;
        }
        void clear() 
        {
            delete [] mRecords;
            mRecords = 0;
            mNrecords = 0;
        }
    };
}
