//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/updatehandler.cpp
// Created by  : Steinberg, 2008
// Description :
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "base/source/updatehandler.h"
#include "base/source/classfactoryhelpers.h"
#include "base/source/fstring.h"

#if SMTG_CPP11_STDLIBSUPPORT
#include <unordered_map>
#else
#include <map>
#endif
#include <deque>
#include <vector>
#include <algorithm>

#define NON_EXISTING_DEPENDENCY_CHECK 0 // not yet
#define CLASS_NAME_TRACKED DEVELOPMENT

using Steinberg::Base::Thread::FGuard;

namespace Steinberg {

DEF_CLASS_IID (IUpdateManager)

namespace Update {
const uint32 kHashSize = (1 << 8); // must be power of 2 (16 bytes * 256 == 4096)
const uint32 kMapSize = 1024 * 10;

//------------------------------------------------------------------------
inline uint32 hashPointer (void* p)
{
	return (uint32)((uint64 (p) >> 12) & (kHashSize - 1));
}

//------------------------------------------------------------------------
inline IPtr<FUnknown> getUnknownBase (FUnknown* unknown)
{
	FUnknown* result = nullptr;
	if (unknown)
	{
		if (unknown->queryInterface (FObject::iid, (void**)&result) != kResultTrue)
			unknown->queryInterface (FUnknown::iid, (void**)&result);
	}

	return owned (result);
}

#if CLASS_NAME_TRACKED
//------------------------------------------------------------------------
struct Dependency
{
	Dependency (FUnknown* o, IDependent* d)
	: obj (o), dep (d), objClass (nullptr), depClass (nullptr)
	{
	}

	inline bool operator== (const Dependency& d) const { return obj == d.obj; }
	inline bool operator!= (const Dependency& d) const { return obj != d.obj; }
	inline bool operator< (const Dependency& d) const { return obj < d.obj; }
	inline bool operator> (const Dependency& d) const { return obj > d.obj; }
	FUnknown* obj;
	IDependent* dep;

	FClassID objClass;
	FClassID depClass;
};
#endif

//------------------------------------------------------------------------
struct DeferedChange
{
	DeferedChange (FUnknown* o, int32 m = 0) : obj (o), msg (m) {}
	~DeferedChange () {}
	DeferedChange (const DeferedChange& r) : obj (r.obj), msg (r.msg) {}
	inline bool operator== (const DeferedChange& d) const { return obj == d.obj; }
	inline bool operator!= (const DeferedChange& d) const { return obj != d.obj; }
	FUnknown* obj;
	int32 msg;
};

//------------------------------------------------------------------------
struct UpdateData
{
	UpdateData (FUnknown* o, IDependent** d, uint32 c)
	: obj (o), dependents (d), count (c)
	{
	}
	FUnknown* obj;
	IDependent** dependents;
	uint32 count;
	bool operator== (const UpdateData& d) const
	{
		return d.obj == obj && d.dependents == dependents;
	}
};

//------------------------------------------------------------------------
using DeferedChangeList = std::deque<DeferedChange>;
using DeferedChangeListIterConst = DeferedChangeList::const_iterator;
using DeferedChangeListIter = DeferedChangeList::iterator;

using UpdateDataList = std::deque<UpdateData>;
using UpdateDataListIterConst = UpdateDataList::const_iterator;

#if CLASS_NAME_TRACKED
using DependentList = std::vector<Dependency>;
#else
typedef std::vector<IDependent*> DependentList;
#endif
using DependentListIter = DependentList::iterator;
using DependentListIterConst = DependentList::const_iterator;

#if SMTG_CPP11_STDLIBSUPPORT
using DependentMap = std::unordered_map<const FUnknown*, DependentList>;
#else
typedef std::map<const FUnknown*, DependentList> DependentMap;
#endif
using DependentMapIter = DependentMap::iterator;
using DependentMapIterConst = DependentMap::const_iterator;

struct Table
{
	DependentMap depMap[kHashSize];
	DeferedChangeList defered;
	UpdateDataList updateData;
};

//------------------------------------------------------------------------
void updateDone (FUnknown* unknown, int32 message)
{
	if (message != IDependent::kDestroyed)
	{
		FObject* obj = FObject::unknownToObject (unknown);
		if (obj)
			obj->updateDone (message);
	}
}
} // namespace Update

//------------------------------------------------------------------------
static int32 countEntries (Update::DependentMap& map)
{
	int32 total = 0;
	Update::DependentMapIterConst iterMap = map.begin ();
	while (iterMap != map.end ())
	{
		const Update::DependentList& list = iterMap->second;
		Update::DependentListIterConst iterList = list.begin ();
		while (iterList != list.end ())
		{
			total++;
			++iterList;
		}
		++iterMap;
	}
	return total;
}

//------------------------------------------------------------------------
UpdateHandler::UpdateHandler ()
{
	table = NEW Update::Table;
	if (FObject::getUpdateHandler () == nullptr)
		FObject::setUpdateHandler (this);
}

//------------------------------------------------------------------------
UpdateHandler::~UpdateHandler ()
{
	if (FObject::getUpdateHandler () == this)
		FObject::setUpdateHandler (nullptr);
	delete table;
	table = nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::addDependent (FUnknown* u, IDependent* _dependent)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (!unknown || !_dependent)
		return kResultFalse;

	FGuard guard (lock);

#if CLASS_NAME_TRACKED
	Update::Dependency dependent (unknown, _dependent);

	FObject* obj = FObject::unknownToObject (unknown);
	if (obj)
		dependent.objClass = obj->isA ();
	obj = FObject::unknownToObject (_dependent);
	if (obj)
		dependent.depClass = obj->isA ();
#else
	IDependent* dependent = _dependent;
#endif

	Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
	Update::DependentMapIter it = map.find (unknown);
	if (it == map.end ())
	{
		Update::DependentList list;
		list.push_back (dependent);
		map[unknown] = list;
	}
	else
	{
		(*it).second.push_back (dependent);
	}

	return kResultTrue;
}
//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::removeDependent (FUnknown* u, IDependent* dependent)
{
	size_t eraseCount;
	return removeDependent (u, dependent, eraseCount);
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::removeDependent (FUnknown* u, IDependent* dependent, size_t& eraseCount)
{
	eraseCount = 0;
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (unknown == nullptr && dependent == nullptr)
		return kResultFalse;

	FGuard guard (lock);

	Update::UpdateDataListIterConst iter = table->updateData.begin ();
	while (iter != table->updateData.end ())
	{
		if ((*iter).obj == unknown || unknown == nullptr)
		{
			for (uint32 count = 0; count < (*iter).count; count++)
			{
				if ((*iter).dependents[count] == dependent)
					(*iter).dependents[count] = nullptr;
			}
		}
		++iter;
	}
	// Remove all objects for the given dependent
	if (unknown == nullptr)
	{
		for (uint32 j = 0; j < Update::kHashSize; j++)
		{
			Update::DependentMap& map = table->depMap[j];
			Update::DependentMapIter iterMap = map.begin ();
			while (iterMap != map.end ())
			{
				Update::DependentList& list = (*iterMap).second;
				Update::DependentListIter iterList = list.begin ();
				bool listIsEmpty = false;
				
				while (iterList != list.end ())
				{
#if CLASS_NAME_TRACKED
					if ((*iterList).dep == dependent)
#else
					if ((*iterList) == dependent)
#endif
					{
						eraseCount = list.size ();
						if (list.size () == 1u)
						{
							listIsEmpty = true;
							break;
						}
						iterList = list.erase (iterList);
					}
					else
					{
						++iterList;
					}
				}
				
				if (listIsEmpty)
					iterMap = map.erase (iterMap);
				else
					++iterMap;
			}
		}
	}
	else
	{
		bool mustFlush = true;

		Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
		Update::DependentMapIter iterList = map.find (unknown);

#if NON_EXISTING_DEPENDENCY_CHECK
		SMTG_ASSERT (iterList != map.end () && "ERROR: Trying to remove a non existing dependency!")
#endif
		if (iterList != map.end ())
		{
			if (dependent == nullptr) // Remove all dependents of object
			{
				eraseCount  = iterList->second.size ();
				map.erase (iterList);
			}
			else // Remove one dependent
			{
				Update::DependentList& dependentlist = (*iterList).second;
				Update::DependentListIter iterDependentlist = dependentlist.begin ();
				while (iterDependentlist != dependentlist.end ())
				{
#if CLASS_NAME_TRACKED
					if ((*iterDependentlist).dep == dependent)
#else
					if ((*iterDependentlist) == dependent)
#endif
					{
						iterDependentlist = dependentlist.erase (iterDependentlist);
						eraseCount++;
						if (dependentlist.empty ())
						{
							map.erase (iterList);
							break;
						}
					}
					else
					{
						++iterDependentlist;
						mustFlush = false;
					}
				}
			}
		}
		if (mustFlush)
			cancelUpdates (unknown);
	}

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult UpdateHandler::doTriggerUpdates (FUnknown* u, int32 message, bool suppressUpdateDone)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (!unknown)
		return kResultFalse;

	// to avoid crashes due to stack overflow, we reduce the amount of memory reserved for the
	// dependents
	IDependent* smallDependents[Update::kMapSize / 10]; // 8kB for x64
	IDependent** dependents = smallDependents;
	int32 maxDependents = Update::kMapSize / 10;
	int32 count = 0;

	{
		FGuard guard (lock); // scope for lock

		Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
		Update::DependentMapIterConst iterList = map.find (unknown);
		if (iterList != map.end ())
		{
			const Update::DependentList& dependentlist = (*iterList).second;
			Update::DependentListIterConst iterDependentlist = dependentlist.begin ();
			while (iterDependentlist != dependentlist.end ())
			{
#if CLASS_NAME_TRACKED
				dependents[count] = (*iterDependentlist).dep;
#else
				dependents[count] = *iterDependentlist;
#endif
				count++;

				if (count >= maxDependents)
				{
					if (dependents == smallDependents)
					{
						dependents = NEW IDependent*[Update::kMapSize];
						memcpy (dependents, smallDependents, count * sizeof (dependents[0]));
						maxDependents = Update::kMapSize;
					}
					else
					{
						SMTG_WARNING ("Dependency overflow")
						break;
					}
				}
				++iterDependentlist;
			}
		}

		// push update data on the stack
		if (count > 0)
		{
			Update::UpdateData data (unknown, dependents, count);
			table->updateData.push_back (data);
		}
	} // end scope

	int32 i = 0;
	while (i < count)
	{
		if (dependents[i])
			dependents[i]->update (unknown, message);
		i++;
	}

	if (dependents != smallDependents)
		delete[] dependents;

	// remove update data from the stack
	if (count > 0)
	{
		FGuard guard (lock);

		table->updateData.pop_back ();
	}

	// send update done message
	if (suppressUpdateDone == false)
		Update::updateDone (unknown, message);

	return count > 0 ? kResultTrue : kResultFalse; // Object was found and has been updated
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::triggerUpdates (FUnknown* u, int32 message)
{
	return doTriggerUpdates (u, message, false);
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::deferUpdates (FUnknown* u, int32 message)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (!unknown)
		return kResultFalse;

	FGuard guard (lock);

	Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
	Update::DependentMapIterConst iterList = map.find (unknown);

	bool hasDependency = (iterList != map.end ());
	if (hasDependency == false)
	{
		Update::updateDone (unknown, message);
		return kResultTrue;
	}

	bool found = false;
	Update::DeferedChangeListIterConst iter = table->defered.begin ();
	while (iter != table->defered.end ())
	{
		if ((*iter).obj == unknown && (*iter).msg == message)
		{
			found = true;
			break;
		}
		++iter;
	}

	if (!found)
	{
		Update::DeferedChange change (unknown, message);
		table->defered.push_back (change);
	}

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::triggerDeferedUpdates (FUnknown* unknown)
{
	Update::DeferedChangeList deferedAgain;
	if (!unknown)
	{
		while (table->defered.empty () == false)
		{
			// Remove first from queue
			lock.lock ();

			FUnknown* obj = table->defered.front ().obj;
			int32 msg = table->defered.front ().msg;
			table->defered.pop_front ();

			// check if this object is currently being updated. in this case, defer update again...
			bool canSignal = true;
			Update::UpdateDataListIterConst iter = table->updateData.begin ();
			while (iter != table->updateData.end ())
			{
				if ((*iter).obj == obj)
				{
					canSignal = false;
					break;
				}
				++iter;
			}
			lock.unlock ();

			if (canSignal)
			{
				triggerUpdates (obj, msg);
			}
			else
			{
				Update::DeferedChange change (obj, msg);
				deferedAgain.push_back (change);
			}
		}
	}
	else
	{
		IPtr<FUnknown> object = Update::getUnknownBase (unknown);
		Update::DeferedChange tmp (object);

		while (true)
		{
			lock.lock ();
			Update::DeferedChangeListIter it =
			    std::find (table->defered.begin (), table->defered.end (), tmp);
			if (it == table->defered.end ())
			{
				lock.unlock ();
				return kResultTrue;
			}

			if ((*it).obj != nullptr)
			{
				int32 msg = (*it).msg;
				table->defered.erase (it);

				// check if this object is currently being updated. in this case, defer update
				// again...
				bool canSignal = true;
				Update::UpdateDataListIterConst iter = table->updateData.begin ();
				while (iter != table->updateData.end ())
				{
					if ((*iter).obj == object)
					{
						canSignal = false;
						break;
					}
					++iter;
				}
				lock.unlock ();

				if (canSignal)
				{
					triggerUpdates (object, msg);
				}
				else
				{
					Update::DeferedChange change (object, msg);
					deferedAgain.push_back (change);
				}
			}
		}
	}

	if (deferedAgain.empty () == false)
	{
		FGuard guard (lock);

		Update::DeferedChangeListIterConst iter = deferedAgain.begin ();
		while (iter != deferedAgain.end ())
		{
			table->defered.push_back (*iter);
			++iter;
		}
	}

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API UpdateHandler::cancelUpdates (FUnknown* u)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (!unknown)
		return kResultFalse;

	FGuard guard (lock);

	Update::DeferedChange change (unknown, 0);
	while (true)
	{
		auto iter = std::find (table->defered.begin (), table->defered.end (), change);
		if (iter != table->defered.end ())
			table->defered.erase (iter);
		else
			break;
	}

	return kResultTrue;
}

//------------------------------------------------------------------------
size_t UpdateHandler::countDependencies (FUnknown* object)
{
	FGuard guard (lock);
	uint32 res = 0;

	IPtr<FUnknown> unknown = Update::getUnknownBase (object);
	if (unknown)
	{
		Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
		Update::DependentMapIter iterList = map.find (unknown);
		if (iterList != map.end ())
			return iterList->second.size ();
	}
	else
	{
		for (uint32 j = 0; j < Update::kHashSize; j++)
		{
			Update::DependentMap& map = table->depMap[j];
			res += countEntries (map);
		}
	}
	return res;
}

#if DEVELOPMENT
//------------------------------------------------------------------------
bool UpdateHandler::checkDeferred (FUnknown* object)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (object);

	FGuard guard (lock);

	Update::DeferedChange tmp (unknown);
	Update::DeferedChangeListIterConst it =
	    std::find (table->defered.begin (), table->defered.end (), tmp);
	if (it != table->defered.end ())
		return true;

	return false;
}

//------------------------------------------------------------------------
bool UpdateHandler::hasDependencies (FUnknown* u)
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (u);
	if (!unknown)
		return false;

	FGuard guard (lock);

	Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
	Update::DependentMapIterConst iterList = map.find (unknown);
	bool hasDependency = (iterList != map.end ());

	return hasDependency;
}

//------------------------------------------------------------------------
void UpdateHandler::printForObject (FObject* obj) const
{
	IPtr<FUnknown> unknown = Update::getUnknownBase (obj);
	if (!unknown)
		return;

	FUnknownPtr<IDependent> dep (obj);

	bool header = false;

	Update::DependentMap& map = table->depMap[Update::hashPointer (unknown)];
	Update::DependentMapIterConst iterList = map.begin ();
	while (iterList != map.end ())
	{
		const Update::DependentList& dependentlist = (*iterList).second;
		Update::DependentListIterConst iterDependentlist = dependentlist.begin ();
		while (iterDependentlist != dependentlist.end ())
		{
#if CLASS_NAME_TRACKED
			if ((*iterList).first == unknown || (*iterDependentlist).dep == dep.getInterface ())
			{
				if (!header)
				{
					FDebugPrint ("Dependencies for object %8" FORMAT_INT64A " %s\n", (uint64)obj,
					             obj->isA ());
					header = true;
				}
				FDebugPrint ("%s %8" FORMAT_INT64A "\n <- %s %8" FORMAT_INT64A "\n",
				             (*iterDependentlist).depClass, (uint64) (*iterDependentlist).dep,
				             (*iterDependentlist).objClass, (uint64) ((*iterList).first));
			}
#else
			if ((*iterList).first == unknown || (*iterDependentlist) == dep.getInterface ())
			{
				if (!header)
				{
					FDebugPrint ("Dependencies for object %8" FORMAT_INT64A " %s\n", (uint64)obj,
					             obj->isA ());
					header = true;
				}
				FDebugPrint ("%8" FORMAT_INT64A "\n <- %8" FORMAT_INT64A "\n",
				             (uint64) (*iterDependentlist), (uint64) ((*iterList).first));
			}
#endif
			++iterDependentlist;
		}

		++iterList;
	}
}
#endif

//------------------------------------------------------------------------
} // namespace Steinberg
