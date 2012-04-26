/*
 * domain.h
 *
 *  Created on: 26 Apr 2010
 *      Author: Holger Schmitz
 */

#ifndef SCHNEK_DOMAIN_H_
#define SCHNEK_DOMAIN_H_

#include "typetools.h"
#include "fixedarray.h"

#include <list>
#include <boost/shared_ptr.hpp>

namespace schnek {

  /** RecDomain is a rectangular domain that is not bound to any concrete grid data structure.
   *
   *  The rectangular domain is defined by a minimum and maximum. An iterator is provided that
   *  traverses the domain and returns the positions.
   */
  template<int rank>
  class RecDomain {
    private:
      /// Minimum and maximum corners of the rectangle
      IndexType min, max;
    public:
      typedef FixedArray<int,rank> IndexType;
      /// Construct with rectangle minimum and maximum
      RecDomain(const IndexType &min_, const IndexType &max_)
      : min(min_), max(max_) {}

      /// Copy constructor
      RecDomain(const RecDomain &domain)
      : min(domain.min), max(domain.max) {}

      /// Assignment operator
      RecDomain &operator=(const RecDomain &domain)
      {
        min = domain.min;
        max = domain.max;
        return *this;
      }

      /// Return rectangle minimum
      const IndexType &getMin() const {return min;}
      /// Return rectangle maximum
      const IndexType &getMax() const {return max;}

      /** Forward iterator over the rectangular domain
       *  Implements operator* and getPos which both return the current iterator position
       */
      class iterator {
        private:
          friend class RecDomain;
          /// Current iterator position
          IndexType pos;
          /// Reference to the rectangular domain
          const RecDomain &domain;
          /// True if the iterator has reached the end
          bool atEnd;

          /// Constructor called by RecDomain to create the iterator
          iterator(const RecDomain &domain_, const IndexType &pos_, bool atEnd_=false)
          : pos(pos_), domain(domain_), atEnd(atEnd_) {}
          /// Default constructor is declared private for now. (Need to implement assignment first)
          iterator();

          /// Increments the iterator by one position.
          void increment()
          {
            int d = rank;
            while (d>0)
            {
              --d;
              if (++pos[d] > domain.getMax()[d])
              {
                pod[d] = domain.getMin()[d];
              }
              else
                return;
            }
            atEnd = true;
          }
        public:
          /// Copy constructor
          iterator(const iterator &it) : pos(it.pos), domain(it.domain), atEnd(it.atEnd) {}

          /// Prefix increment. Increments the iterator by one position.
          iterator &operator++()
          {
            increment();
            return *this;
          }
          /// Postfix increment. Increments the iterator by one position.
          const iterator operator++(int)
          {
            iterator it(*this);
            increment();
            return it;
          }
          /// Equality test
          bool operator==(const iterator &it)
          {
            return (atEnd==it.atEnd) && (pos==it.pos);
          }
          /// Returns the current iterator position
          const IndexType& operator*() { return pos; }

          /// Returns the current iterator position
          const IndexType& getPos() { return pos; }
      };

      /// Creates an iterator pointing to the beginning of the rectangle
      iterator begin() {
        return iterator(*this, min);
      }

      /// Creates an iterator pointing to a position after the end of the rectangle
      iterator end() {
        return iterator(*this, min, true);
      }
  };

  typedef boost::shared_ptr<RecDomain> pRecDomain;

  /** MultiRecDomain contains a list of RecDomain rectangular domains.
   *
   *  New domains can be added by supplying the rectangular domain bounds.
   */
  template<int rank>
  class MultiRecDomain {
    private:
      /// The list of rectangular domains
      std::list<pRecDomain> domains;

    public:
      /// Default constructor
      MultiRecDomain() {}

      /// Copy constructor
      MultiRecDomain(const MultiRecDomain &dom)
      : domains(dom.domains) {}

      /// Add a new rectangular domain by supplying the domain bounds.
      const Domain &addDomain(const IndexType &min_, const IndexType &max_)
      {
        pDomain ndom(new Domain(min_, max_));
        domains.push_back(ndom);
        return *ndom;
      }
  };

  typedef boost::shared_ptr<MultiRecDomain> pMultiRecDomain;

} // namespace schnek


#endif /* SCHNEK_DOMAIN_H_ */
