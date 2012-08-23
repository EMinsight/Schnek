/*
 * dependencies.cpp
 *
 * Created on: 10 Jul 2012
 * Author: Holger Schmitz
 * Email: holger@notjustphysics.com
 *
 * Copyright 2012 Holger Schmitz
 *
 * This file is part of Schnek.
 *
 * Schnek is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Schnek is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Schnek.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "dependencies.hpp"
#include "expression.hpp"
#include "../exception.hpp"

#include <boost/variant/apply_visitor.hpp>
#include <boost/foreach.hpp>
#include <cassert>

using namespace schnek;

DependencyMap::DependencyMap(const pBlockVariables vars)
{
  blockVars = vars;
  constructMap(vars);
}

void DependencyMap::constructMapRecursive(const pBlockVariables vars)
{
  BOOST_FOREACH(VariableMap::value_type it, vars->getVariables())
  {
    pVariable v=it.second;
    if (!v->isConstant())
    {
      DependenciesGetter depGet;
      DependencySet dep = boost::apply_visitor(depGet, v->getExpression());
      long id = v->getId();
      if (dependencies.count(id)>0) throw SchnekException();
      dependencies[id] = VarInfo(v, dep, DependencySet());
    }
  }

  BOOST_FOREACH(pBlockVariables ch, vars->getChildren())
  {
    constructMapRecursive(ch);
  }
}
void DependencyMap::constructMap(const pBlockVariables vars)
{
	constructMapRecursive(vars);

	BOOST_FOREACH(DepMap::value_type entry, dependencies)
	{
	  BOOST_FOREACH(long id, entry.second.dependsOn)
	  {
	    dependencies[id].modifies.insert(entry.first);
	  }
	}
}

void DependencyMap::resetCounters()
{
	BOOST_FOREACH(DepMap::value_type entry, dependencies)
	{
	  entry.second.counter = entry.second.dependsOn.size();
	}
}

void DependencyMap::makeUpdateList(const VariableSet &independentVars, const VariableSet &dependentVars, VariableList &updateList)
{
  pRefDepMap reverseDeps = makeUpdatePredecessors(dependentVars);
  pRefDepMap deps = makeUpdateFollowers(independentVars, reverseDeps);
  makeUpdateOrder(independentVars, deps, updateList);
}

DependencyMap::pRefDepMap DependencyMap::makeUpdatePredecessors(const VariableSet &dependentVars)
{
  std::list<VarInfo*> workingSet;
  pRefDepMap predecessors_p(new RefDepMap());
  RefDepMap &predecessors = *predecessors_p; // only used for more readable code

  // first initialise the working set and also add the variables to the predecessors
  BOOST_FOREACH(pVariable v, dependentVars)
  {
    long id=v->getId();
    VarInfo *vi = &(dependencies[id]);
    workingSet.push_back(vi);
    predecessors[id] = vi;
  }

  // take elements out of the working set and analyse their predecessors
  while (!workingSet.empty())
  {
    VarInfo *vi = workingSet.front();
    workingSet.pop_front();

    BOOST_FOREACH(long id, vi->dependsOn)
    {
      // check if we already considered this variable
      if (predecessors.count(id) > 0) continue;

      // if not, put predecessors in the working set and in the map
      VarInfo *pred = &(dependencies[id]);
      workingSet.push_back(pred);
      predecessors[id] = pred;
    }
  }

  // The map now contains all the predecessors of the dependent variables but note
  // that the VarInfo::modifies sets are not restricted to this map.
  return predecessors_p;

}

DependencyMap::pRefDepMap DependencyMap::makeUpdateFollowers(const VariableSet &independentVars, pRefDepMap predecessors_p)
{
  std::list<VarInfo*> workingSet;
  pRefDepMap followers_p(new RefDepMap());
  RefDepMap &followers = *followers_p;
  RefDepMap &predecessors = *predecessors_p;

  // first initialise the working set and also add the variables to the followers
  BOOST_FOREACH(pVariable v, independentVars)
  {
    long id=v->getId();
    // the variable is not needed if it is not in the predecessors
    if (predecessors.count(id) == 0) continue;
    VarInfo *vi = predecessors[id];
    workingSet.push_back(vi);
    followers[id] = vi;
  }

  // take elements out of the working set and analyse their predecessors
  while (!workingSet.empty())
  {
    VarInfo *vi = workingSet.front();
    workingSet.pop_front();

    BOOST_FOREACH(long id, vi->modifies)
    {
      // check if we already considered this variable or if we need it at all
      if ((followers.count(id) > 0) || (predecessors.count(id) == 0)) continue;

      // if not, put predecessors in the working set and in the map
      VarInfo *foll = predecessors[id];
      workingSet.push_back(foll);
      followers[id] = foll;
    }
  }

  // The map now contains all the predecessors of the dependent variables but note
  // that the VarInfo::modifies sets are not restricted to this map.
  return followers_p;

}

void DependencyMap::makeUpdateOrder(const VariableSet &independentVars, pRefDepMap deps_p, VariableList &updateList)
{
  typedef std::list<VarInfo*>::iterator WorkIter;

  std::list<VarInfo*> workingSet;
  RefDepMap &deps = *deps_p;

  updateList.clear();

  // first initialise the counters
  BOOST_FOREACH(RefDepMap::value_type entry, deps)
  {
    int count = 0;
    VarInfo *vi = entry.second;
    workingSet.push_back(vi);
    BOOST_FOREACH(long id, vi->dependsOn)
      if (deps.count(id) > 0) ++count;

    entry.second->counter = count;
  }

  while (!workingSet.empty())
  {
    VarInfo *nextEval = 0;
    WorkIter it = workingSet.begin();
    while ((nextEval==0) && (it!=workingSet.end()))
    {
      if ((*it)->counter==0) nextEval=*it;
      else ++it;
    }

    assert(nextEval != 0);

    workingSet.erase(it);
    updateList.push_back(nextEval->v);

    BOOST_FOREACH(long id, nextEval->modifies)
    {
      if (deps.count(id) == 0) continue;
      VarInfo *vi = deps[id];
      --(vi->counter);
      assert(vi->counter >= 0);
    }
  }
}

DependencyUpdater::DependencyUpdater(pDependencyMap dependencies_)
  : dependencies(dependencies_), isValid(true)
{}

pBlockVariables DependencyMap::getBlockVariables()
{
  return blockVars;
}

void DependencyUpdater::addIndependent(pVariable v)
{
  independentVars.insert(v);
  isValid = false;
}

void DependencyUpdater::addDependent(pVariable v)
{
  dependentVars.insert(v);
  isValid = false;
}

