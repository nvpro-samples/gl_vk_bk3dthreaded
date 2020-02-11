/*-----------------------------------------------------------------------
    Copyright (c) 2016, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------

#ifndef ThreadTest_RingBuffer_h
#define ThreadTest_RingBuffer_h

//#pragma mark - Ring Buffer // MacOSX thing
#include <algorithm>
#include <cstddef>

/******************************************************************************/
/**
 ** \brief Ring buffer implementation "a la STL"...
 **/
template <typename T, class Alloc = std::allocator<T> >
class NRingBuffer
{
public:
    
    template <typename V>
    class iterator_impl : public std::iterator<std::forward_iterator_tag, T>
    {
        friend class NRingBuffer<T>;
        V* m_buffer;
        size_t m_capacity;
        V* m_ptr;
        //if the buffer is full end (one after the last)  will == begin which means empty...we need to fake this
        mutable bool m_isFreshBeginAndIsFull;
        mutable bool m_isFreshEndAndIsFull;
        iterator_impl(V* b, size_t c, V* curPtr, bool freshB, bool freshE) : 
        m_buffer(b), m_capacity(c), m_ptr(curPtr), m_isFreshBeginAndIsFull(freshB), m_isFreshEndAndIsFull(freshE) 
        {
        };
    public:
        iterator_impl(const iterator_impl<V>& i) : m_buffer(i.m_buffer), m_capacity(i.m_capacity), 
        m_ptr(i.m_ptr), m_isFreshBeginAndIsFull(i.m_isFreshBeginAndIsFull), m_isFreshEndAndIsFull(i.m_isFreshEndAndIsFull) 
        {
        };
        iterator_impl& operator++()
        { 
            m_isFreshBeginAndIsFull = false; //once you move them they are not fresh
            m_isFreshEndAndIsFull = false;
            m_ptr++;
            assert(m_ptr <= (m_buffer + m_capacity));
            if (m_ptr == (m_buffer + m_capacity))
                m_ptr = m_buffer;
            
            return *this;
        }
        iterator_impl operator++(int) 
        {
            iterator_impl<V> tmp(*this); 
            operator++(); 
            return tmp;
        }
        
        iterator_impl& operator--()
        { 
            assert(m_ptr >= m_buffer);
            m_isFreshBeginAndIsFull = false;
            m_isFreshEndAndIsFull = false;
            m_ptr--;
            if (m_ptr == (m_buffer - 1))
                m_ptr += m_capacity;
            
            return *this;
        }
        iterator_impl operator--(int) 
        {
            iterator_impl<V> tmp(*this); 
            operator--(); 
            return tmp;
        }
        
        bool operator==(const iterator_impl<V>& rhs) const 
        {
            if (m_ptr==rhs.m_ptr)
            {
                if ((m_isFreshBeginAndIsFull && rhs.m_isFreshEndAndIsFull)
                    || (m_isFreshEndAndIsFull && rhs.m_isFreshBeginAndIsFull))
                    return false; //if we are full then initally ptr begin == ptr end and that violates how iterators work
                else
                    return true;
            }
            else
            {
                return false;
            }
        }
        bool operator!=(const iterator_impl<V>& rhs) const 
        {
            return !(operator==(rhs));
        }
        V& operator*() const 
        {
            return *m_ptr;
        }
        
        //convert to non-const
        operator iterator_impl<T>() 
        {
            return iterator_impl<T>(m_buffer, m_capacity, m_ptr, m_isFull, m_isFreshBeginAndIsFull, m_isFreshEndAndIsFull); 
        }
    };
    
    template <typename V> 
    friend class iterator_impl;
    
    //typedefs to make our class stl-compliant
    typedef typename Alloc::reference reference;
    typedef typename Alloc::const_reference const_reference;
    typedef iterator_impl<const T> const_iterator;
    typedef iterator_impl<T> iterator;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef Alloc allocator_type;
    typedef typename Alloc::pointer pointer;
    typedef typename Alloc::const_pointer const_pointer;
    
    
    iterator begin() 
    {
        return iterator(m_buffer, m_capacity, m_readPtr, m_isFull, false);
    }
    
    iterator end()
    {
        if (m_capacity > 0)
            return iterator(m_buffer, m_capacity, (((m_readPtr - m_buffer)  + GetStoredSize()) % m_capacity) + m_buffer, false, m_isFull);
        else
            return begin();
    }
    
    const_iterator begin() const
    {
        return const_iterator(m_buffer, m_capacity, m_readPtr, m_isFull, false);
    }
    
    const_iterator end() const
    {
        if (m_capacity > 0)
            return const_iterator(m_buffer, m_capacity, (((m_readPtr - m_buffer)  + GetStoredSize()) % m_capacity) + m_buffer, false, m_isFull);
        else
            return begin();
    }
    
    enum OVERFLOW_BEHAVIOR
    {
        OF_FAIL, //return failure and do nothing
        OF_DISCARD, //overwrite the oldest data
        OF_EXPAND, //resize
    };
    
    NRingBuffer(size_t capacity, OVERFLOW_BEHAVIOR  overflow = OF_EXPAND, const Alloc& alloc = Alloc()):
    m_overflowBehavoir(overflow), m_capacity(capacity), m_isFull(false), m_allocator(alloc)
    {
        m_buffer = m_capacity ? m_allocator.allocate(m_capacity) : NULL;
        m_readPtr = m_writePtr = m_buffer;
    }
    
    NRingBuffer(const NRingBuffer<T, Alloc>& src) :
    m_overflowBehavoir(src.m_overflowBehavoir), m_capacity(src.m_capacity), m_isFull(src.m_isFull), m_allocator(src.m_allocator)
    {
        CopyFrom(src);
    }
    
    NRingBuffer<T, Alloc>& operator=(const NRingBuffer<T, Alloc> &rhs)
    {
        if (this == &rhs)
            return *this;
        
        KillBuffer();
        CopyFrom(rhs);
        return *this;
    }
    
    
    ~NRingBuffer()
    {
        KillBuffer();
    }
    
    size_t GetCapacity() const
    {
        return m_capacity;
    }
    
    void Reset(size_t newCapacity)
    {
        if (newCapacity != m_capacity)
        {
            KillBuffer();
            m_capacity = newCapacity;
            m_buffer = m_capacity ? m_allocator.allocate(m_capacity) : NULL;
        }
        m_readPtr = m_writePtr = m_buffer;
        m_isFull = false;
    }
    
    void Reset()
    {
        Reset(GetCapacity());
    }
    
    size_t GetFreeSize() const
    {
        return m_capacity - GetStoredSize();
    }
    
    size_t GetStoredSize() const
    {
        if (m_writePtr < m_readPtr) //it wraps around
        {
            size_t rightStored = (m_buffer + m_capacity) - m_readPtr; //stuff after the read pointer
            size_t leftStored  = m_writePtr - m_buffer; //stuff before the end read pointer
            return rightStored + leftStored;
        }
        else
        {
            return m_isFull ? m_capacity : (m_writePtr - m_readPtr);
        }
    }
    
    bool WriteData(const T* src, size_t len)
    {
        size_t freeSize = GetFreeSize();
        if (len > freeSize)
        {
            switch(m_overflowBehavoir)
            {
                case OF_FAIL:
                {
                    return false; //just fail
                }
                case OF_DISCARD:
                {
                    if (len > m_capacity) //if its larget than our total size or would over
                        return false;
                    ConsumeData(len - freeSize);
                    break;
                }
                case OF_EXPAND:
                {
                    size_t storedSize = GetStoredSize();
                    size_t newCap = m_capacity * 2;
                    while((len + storedSize) > newCap)
                    {
                        newCap *= 2;
                    }
                    T* newBuffer = m_allocator.allocate(newCap);
                    ReadDataImpl(newBuffer, storedSize, true); //copy over data calling constructors
                    T* newRead = newBuffer;
                    T* newWrite = newBuffer + storedSize;
                    
                    KillBuffer();
                    
                    m_buffer = newBuffer;
                    m_writePtr = newWrite;
                    m_readPtr = newRead;
                    m_capacity = newCap;
                    //NOutputString("Expanded\n");
                }
                    break;
            }
        }
        
        size_t rightFree = (m_writePtr >= m_readPtr && !m_isFull) ? (m_buffer + m_capacity) - m_writePtr : m_readPtr - m_writePtr; //stuff after the write pointer
        size_t leftFree  = (m_writePtr >= m_readPtr && !m_isFull) ? m_readPtr - m_buffer : 0; //stuff before the read pointer
        
        size_t writeAmt = rightFree < len ? rightFree : len;
        for (size_t i = 0; i < writeAmt; i++)
        {
            m_allocator.construct(&m_writePtr[i], src[i]); //copy construct the objects
        }
        m_writePtr += writeAmt;
        
        assert(m_writePtr <= (m_buffer + m_capacity));
        if (m_writePtr == (m_buffer + m_capacity))
            m_writePtr = m_buffer;
        
        if (len > writeAmt)
        {
            assert((len - writeAmt) <= leftFree);
            for (size_t i = 0; i < (len-writeAmt); i++)
            {
                m_allocator.construct(&m_writePtr[i], src[i + writeAmt]); //copy construct the objects
            }
            m_writePtr += (len - writeAmt);
        }
        
        m_isFull = (m_writePtr == m_readPtr && (len || m_isFull));
        return true;
    }
    
    bool WriteData(const T& d)
    {
        return WriteData(&d, 1);
    }
    
    size_t ReadData(T* dest, size_t destSize)
    {
        return ReadDataImpl(dest, destSize, false);
    }
    
    bool ReadData(T& dest)
    {
        return ReadData(&dest, 1) > 0;
    }
    
    size_t ConsumeData(size_t count)
    {
        return ReadData(NULL, count);
    }
    
    //reads data but doesn't remove it
    size_t PeekData(T* dest, size_t destSize, size_t offset = 0) const
    {
        //spoof the offset as a largeer buffer
        destSize += offset;
        
        size_t rightStored = (m_writePtr < m_readPtr || m_isFull) ? (m_buffer + m_capacity) - m_readPtr : m_writePtr - m_readPtr; //stuff after the read pointer
        size_t leftStored  = (m_writePtr < m_readPtr || m_isFull) ? m_writePtr - m_buffer : 0; //stuff before the end read pointer
        
        size_t readFromRightSize = std::min(rightStored, destSize);
        if (dest && offset <= readFromRightSize)
            dest = std::copy(m_readPtr + offset, m_readPtr + readFromRightSize, dest); //readFromRightSize has offset baked in
        
        T* nextPtr =  m_readPtr + readFromRightSize;
        
        destSize -= readFromRightSize;
        size_t readFromLeftSize = std::min(leftStored, destSize);
        
        assert(nextPtr <= (m_buffer + m_capacity));
        if (nextPtr == (m_buffer + m_capacity))
            nextPtr = m_buffer;
        
        if (dest)
            std::copy(nextPtr, nextPtr + readFromLeftSize, dest);
        
        size_t readAmt = readFromRightSize + readFromLeftSize - offset;
        return readAmt;
    }
    
    bool PeekData(T& dest, size_t offset = 0) const
    {
        return PeekData(&dest, 1, offset) > 0;
    }
    
private:
    T* m_buffer;
    T* m_writePtr;
    T* m_readPtr;
    
    size_t m_capacity;
    bool m_isFull;
    
    OVERFLOW_BEHAVIOR m_overflowBehavoir;
    Alloc m_allocator;
    
    size_t ReadDataImpl(T* dest, size_t destSize, bool construct)
    {
        size_t rightStored = (m_writePtr < m_readPtr || m_isFull) ? (m_buffer + m_capacity) - m_readPtr : m_writePtr - m_readPtr; //stuff after the read pointer
        size_t leftStored  = (m_writePtr < m_readPtr || m_isFull) ? m_writePtr - m_buffer : 0; //stuff before the end read pointer
        
        size_t readFromRightSize = (rightStored < destSize) ? rightStored : destSize;
        
        for (size_t i = 0; i < readFromRightSize; i++)
        {
            if (dest)
            {
                if (construct)
                    m_allocator.construct(&dest[i], m_readPtr[i]);
                else
                    dest[i] = m_readPtr[i];
            }
            
            m_allocator.destroy(&m_readPtr[i]);
        }
        
        
        m_readPtr += readFromRightSize;
        
        destSize -= readFromRightSize;
        size_t readFromLeftSize = (leftStored < destSize) ? leftStored : destSize;
        
        assert(m_readPtr <= (m_buffer + m_capacity));
        if (m_readPtr == (m_buffer + m_capacity))
            m_readPtr = m_buffer;
        
        if (readFromLeftSize > 0)
        {
            for (size_t i = 0; i < readFromLeftSize; i++)
            {
                if (dest)
                {
                    if (construct)
                        m_allocator.construct(&dest[i+readFromRightSize], m_readPtr[i]);
                    else
                        dest[i+readFromRightSize] = m_readPtr[i];
                }
                
                m_allocator.destroy(&m_readPtr[i]);
            }
            
            m_readPtr += readFromLeftSize;
        }
        
        size_t readAmt = readFromRightSize + readFromLeftSize;
        m_isFull = (readAmt == 0 && m_isFull);
        return readAmt;
    }
    
    void KillBuffer()
    {
        iterator e = end();
        for (iterator i = begin(); i != e; i++)
            m_allocator.destroy(&(*i));
        
        m_allocator.deallocate(m_buffer, m_capacity);
    }
    
    void CopyFrom(const NRingBuffer<T, Alloc>& src)
    {
        m_overflowBehavoir = src.m_overflowBehavoir;
        m_capacity = src.m_capacity;
        m_isFull = src.m_isFull;
        m_allocator = src.m_allocator;
        
        m_buffer = m_capacity ? m_allocator.allocate(m_capacity) : NULL;
        m_readPtr = (src.m_readPtr - src.m_buffer) + m_buffer;
        m_writePtr = (src.m_writePtr - src.m_buffer) + m_buffer;
        const_iterator citb = src.begin();
        const_iterator cite = src.end();
        
        iterator itb = begin();
        iterator ite = end();
        while(citb != cite)
        {
            m_allocator.construct(&(*itb), *citb);
            itb++;
            citb++;
        }
    }
};


#endif
