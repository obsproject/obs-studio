/* SPDX-License-Identifier: MIT */
/**
	@file		ajarefptr.h
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
	@brief		Defines the AJARefPtr template class.
**/

#if !defined (__AJAREFPTR__)
	#define __AJAREFPTR__

#include "../system/atomic.h"

	/**
		@brief	I am the referent object that maintains the reference count and
				the pointer to the underlying object whose lifespan I manage.
		@note	I am not designed to be used as a superclass, therefore, none of
				my methods are virtual.
	**/
	template <class TRef>
	class Referent
	{
		//	Instance Methods
		public:
			//	Constructor
			Referent (TRef * ptr);

			//	Reference Count Management
			void AddRef ()		throw ();
			void RemoveRef ()	throw ();

			//	Access To Underlying Object
			TRef * get () const throw();

		//	Instance Data
		private:
			uint32_t	m_nCount;		//	Reference count
			TRef *		m_pointer;		//	Pointer to underlying object I refer to

	};	//	Referent


	template <class TRef>
	Referent <TRef>::Referent (TRef * ptr)
		:	m_nCount (1),
			m_pointer (ptr)
	{
	}


	template <class TRef>
	void Referent <TRef>::AddRef (void) throw ()
	{
		AJAAtomic::Increment(&m_nCount);
	}


	template <class TRef>
	void Referent <TRef>::RemoveRef (void) throw ()
	{
		if (m_nCount > 0)
		{
			if (AJAAtomic::Decrement(&m_nCount) == 0)
			{
				delete m_pointer;
				m_pointer = 0;
				delete this;
			}
		}
	}


	template <class TRef>
	TRef * Referent <TRef>::get (void) const throw ()
	{
		return m_pointer;
	}



	/**
		@brief	I am a reference-counted pointer template class.
				I am intended to be a proxy for an underlying object,
				whose lifespan is automatically managed by the Referent
				template class that I work with.
		@note	Be sure not to create more than one of me using the same
				object pointer or else a double-free will ensue.
	**/
	template <class TRef>
	class AJARefPtr
	{
		public:
			typedef TRef element_type;

			//	Construction & Destruction
			explicit AJARefPtr (TRef * ptr = NULL) throw ();
			AJARefPtr (const AJARefPtr<TRef>& obToCopy ) throw ();
			~AJARefPtr () throw ();

			//	Assignment
			AJARefPtr<TRef>& operator = (const AJARefPtr<TRef>& inRHS) throw ();
			AJARefPtr<TRef>& operator = (TRef * pobRHS) throw ();

			//	Comparison
			bool operator == (const AJARefPtr< TRef > & inRHS) const throw ();
			bool operator != (const AJARefPtr< TRef > & inRHS) const throw ();
			bool operator < (const AJARefPtr< TRef > & inRHS) const throw ();

			//	Dereferencing
			TRef & operator * () const throw ();
			TRef * operator -> () const throw ();
			TRef * get () const throw ();

			//	Testing
			operator bool () const throw ();

		private:
			Referent <element_type> *	m_pRef;		//	My referent

	};	//	AJARefPtr


	template <class TRef>
	AJARefPtr <TRef>::AJARefPtr (TRef * ptr) throw () : m_pRef (new Referent <element_type> (ptr))
	{
	}


	template <class TRef>
	AJARefPtr <TRef>::AJARefPtr (const AJARefPtr <TRef> & obToCopy) throw () : m_pRef (obToCopy.m_pRef)
	{
		m_pRef->AddRef ();
	}


	template <class TRef>
	AJARefPtr <TRef> & AJARefPtr <TRef>::operator = (const AJARefPtr <TRef> & inRHS) throw ()
	{
		if (*this != inRHS)
		{
			if (m_pRef)
				m_pRef->RemoveRef ();
			m_pRef = inRHS.m_pRef;
			m_pRef->AddRef ();
		}
		return *this;
	}


	template <class TRef>
	AJARefPtr <TRef> & AJARefPtr <TRef>::operator = (TRef * pobRHS) throw ()
	{
		if (pobRHS != get ())
		{
			if (m_pRef)
				m_pRef->RemoveRef ();
			m_pRef = new Referent <element_type> (pobRHS);
		}
		return *this;
	}


	template <class TRef>
	AJARefPtr <TRef>::~AJARefPtr () throw ()
	{
		m_pRef->RemoveRef ();
	}


	template <class TRef>
	bool AJARefPtr <TRef>::operator == (const AJARefPtr <TRef> & inRHS) const throw ()
	{
		return get() == inRHS.get();
	}


	template <class TRef>
	bool AJARefPtr <TRef>::operator != (const AJARefPtr <TRef> & inRHS) const throw ()
	{
		return get() != inRHS.get ();
	}


	template <class TRef>
	bool AJARefPtr <TRef>::operator < (const AJARefPtr <TRef> & inRHS) const throw ()
	{
		return get() < inRHS.get ();
	}


	template <class TRef>
	TRef & AJARefPtr <TRef>::operator * () const throw ()
	{
		return *(m_pRef->get());
	}


	template <class TRef>
	TRef * AJARefPtr <TRef>::operator -> () const throw ()
	{
		return m_pRef ? m_pRef->get () : NULL;
	}


	template <class TRef>
	TRef * AJARefPtr <TRef>::get () const throw ()
	{
		return m_pRef ? m_pRef->get () : NULL;
	}


	template <class TRef>
	AJARefPtr <TRef>::operator bool () const throw ()
	{
		return m_pRef && m_pRef->get () != 0;
	}

#endif	//	__AJAREFPTR__
