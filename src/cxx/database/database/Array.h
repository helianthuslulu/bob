/**
 * @file database/src/Array.h
 * @author <a href="mailto:andre.anjos@idiap.ch">Andre Anjos</a> 
 *
 * The Array is the basic unit containing data in a Dataset 
 */

#ifndef TORCH_DATABASE_ARRAY_H 
#define TORCH_DATABASE_ARRAY_H

#include <cstdlib>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <blitz/array.h>

#include "database/ArrayCodec.h"
#include "database/InlinedArrayImpl.h"
#include "database/ExternalArrayImpl.h"

namespace Torch {   
  /**
   * \ingroup libdatabase_api
   * @{
   *
   */
  namespace database {

    //I promise this exists:
    //class Arrayset;

    /**
     * The array class for a dataset. The Array class acts like a manager for
     * the underlying data (blitz::Array<> in memory or serialized in file). It
     * keeps tight its relationship with its parent Arrayset object, if one was
     * assigned.
     */
    class Array {

      public:

        /**
         * Starts a new array with in-memory content. We don't ever copy the
         * data, just refer to it. If you want me to have a private copy, just
         * copy the data before-hand. Please note this constructor is able to
         * receive blitz::Array<> elements by implicit construction into
         * InlinedArrayImpl.
         */
        Array(const detail::InlinedArrayImpl& data);

        /**
         * Builds an Array that contains data from a file. You can optionally
         * specify the name of a codec.
         */
        Array(const std::string& filename, const std::string& codec="");

        /**
         * Refers to the Array data from another array. If a parent was already
         * set, the id property of this copy will be reset according to the
         * availability of ids in the parent.
         */
        Array(const Array& other);

        /**
         * Destroys this array. If this array had a parent, this array is
         * firstly unregistered from the parent and then destroyed together
         * with its data.
         */
        virtual ~Array();

        /**
         * Copies the data from another array. If a parent was already set, the
         * id property of this copy will be reset according to the availability
         * of ids in the parent.
         */
        Array& operator= (const Array& other);

        /**
         * Saves this array in the given path using the codec indicated (or by
         * looking at the file extension if that is empty). If the array was
         * already in a file it is moved/re-encoded as need to fulfill this
         * request. If the array was in memory, it is serialized, from the data
         * I have in memory and subsequently erased. If the filename specifies
         * an existing file, this file is overwritten.
         */
        void save(const std::string& filename, const std::string& codecname="");

        /**
         * If the array is in-memory, returns a reference to it. If it is in a
         * file, the file data is read and I become an inlined array. The
         * underlying file containing the data is <b>not</b> erased, we just
         * unlink it from this Array. If you want to read the array data from
         * the file without switching the internal representation of this array
         * (from external to inlined), use the get() method.
         */
        template <typename T, int D> blitz::Array<T,D> load();

        /**
         * If the array is already in memory, we return a reference to it. If
         * it is in a file, we load it and return a reference to the loaded
         * data.
         */
        template <typename T, int D> blitz::Array<T,D> get() const;

        /**
         * This is a non-templated version of the get() method that returns a
         * generic array, used for typeless manipulations. 
         *
         * @warning You do NOT want to use this!
         */
        detail::InlinedArrayImpl get() const;

        /**
         * Sets the current data to the given array
         */
        void set(const detail::InlinedArrayImpl& data);

        /**
         * Returns the current number of dimensions set by this array.
         */
        size_t getNDim() const;

        /**
         * Returns the type of element of this array.
         */
        Torch::core::array::ElementType getElementType() const; 

        /**
         * Returns the shape of the current array.
         */
        const size_t* getShape() const; 

        /**
         * Get the filename containing the data if any. An empty string
         * indicates that the data is stored inlined.
         */
        const std::string& getFilename() const;

        /**
         * Get the codec used to read the data from the external file 
         * if any. This will be non-empty only if the filename is non-empty.
         */
        boost::shared_ptr<const ArrayCodec> getCodec() const; 

        /**
         * Gets the id of the Array
         */
        inline size_t getId() const { return m_id; }

        /**
         * Sets the id for this Array. If this array has a parent Arrayset, we
         * do check the identity is not taken already. If so, an exception is
         * raised. You can get automatic id's that are guaranteed to be free by
         * setting the special value '0' (zero).
         */
        void setId(size_t id);

        /**
         * Get the flag indicating if the array is loaded in memory
         */
        inline bool isLoaded() const { return m_inlined; }

        /**
         * Sets the parent arrayset of this array. If my parent is already set,
         * I'll detach myself from my old parent and insert myself onto the new
         * parent, checking initialized type information if any was already
         * set. Incompatibilities will be flagged by exceptions.
         *
         * The optional parameter 'id' will set my id property, but first I'll
         * check if that id is unblocked in the parent. If that is not the
         * case, I'll raise an exception. If you don't set the id property in
         * this call, I'll ask the parent to assign me a proper available id.
         */
        /**
        void setParent (boost::shared_ptr<Arrayset> parent, size_t id=0);
        */

        /**
         * Gets the parent arrayset of this array
         */
        /**
        inline boost::shared_ptr<const Arrayset> getParent() const 
        { return m_parent_arrayset.lock(); }
        */
        
      private: //representation
        //boost::weak_ptr<Arrayset> m_parent_arrayset; ///< My current parent
        boost::shared_ptr<detail::InlinedArrayImpl> m_inlined;
        boost::shared_ptr<detail::ExternalArrayImpl> m_external;
        size_t m_id; ///< This is my id
        mutable size_t m_tmp_shape[Torch::core::array::N_MAX_DIMENSIONS_ARRAY]; ///< temporary variable used to return the shape of external arrays.
    };

    template <typename T, int D> blitz::Array<T,D> Array::load() {
      if (D != getNDim()) throw DimensionError();
      if (!m_inlined) {
        m_inlined.reset(new detail::InlinedArrayImpl(m_external->load()));
        m_external.reset();
      }
      return m_inlined->cast<T,D>(); 
    }

    template <typename T, int D> blitz::Array<T,D> Array::get() const {
      if (D != getNDim()) throw DimensionError();
      if (!m_inlined) return m_external->load<T,D>();
      return m_inlined->castCopy<T,D>(); 
    }

  } //closes namespace database

} //closes namespace Torch

#endif /* TORCH_DATABASE_ARRAY_H */
