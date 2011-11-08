/**
 * @file io/File.h
 * @author <a href="mailto:andre.anjos@idiap.ch">Andre Anjos</a> 
 *
 * @brief Describes a generic API for reading and writing data to external
 * files.
 */

#ifndef TORCH_IO_FILE_H 
#define TORCH_IO_FILE_H

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "core/array.h"

namespace Torch { namespace io {
      
  /**
   * Files deal with reading and writing multiple (homogeneous) array data to
   * and from files.
   */
  class File {

    public:

      virtual ~File();

      /**
       * The filename this array codec current points to
       */
      virtual const std::string& filename() const =0;

      /**
       * The typeinfo of data within this file, if it is supposed to be read as
       * a single array.
       */
      virtual const Torch::core::array::typeinfo& array_type() const =0;

      /**
       * The typeinfo of data within this file, if it is supposed to be read as
       * as an array set.
       */
      virtual const Torch::core::array::typeinfo& arrayset_type() const =0;

      /**
       * The number of arrays available in this file, if it is supposed to be
       * read as an array set.
       */
      virtual size_t arrayset_size() const =0;

      /**
       * Returns the name of the codec, for compatibility reasons.
       */
      virtual const std::string& name() const =0;

      /**
       * Loads all the data available at the file into memory.
       *
       * This method will check to see if the given array has enough space. If
       * that is not the case, it will allocate enough space internally by
       * reseting the input array and putting the data read from the file
       * inside.
       */
      virtual void array_read(Torch::core::array::interface& buffer) =0;

      /**
       * Loads the data of the array into memory. If an index is specified
       * loads the specific array data from the file, otherwise, loads the data
       * at position 0.
       *
       * This method will check to see if the given array has enough space. If
       * that is not the case, it will allocate enough space internally by
       * reseting the input array and putting the data read from the file
       * inside.
       */
      virtual void arrayset_read(Torch::core::array::interface& buffer, 
          size_t index) =0;

      /**
       * Appends the given buffer into a file. If the file does not exist,
       * create a new file, else, makes sure that the inserted array respects
       * the previously set file structure.
       *
       * Returns the current position of the newly written array.
       */
      virtual size_t arrayset_append 
        (const Torch::core::array::interface& buffer) =0;

      /**
       * Writes the data from the given buffer into the file and act like it is
       * the only piece of data that will ever be written to such a file. Not
       * more data appending may happen after a call to this method.
       */
      virtual void array_write (const Torch::core::array::interface& buffer) =0;

  };

  /**
   * This defines the factory method F that can create codecs. Your task, as a
   * codec developer is to create one of such methods for each of your codecs
   * and statically register them to the codec registry.
   *
   * Here are the meanings of the mode flag that should be respected by your
   * factory implementation:
   *
   * 'r': opens for reading only - no modifications can occur; it is an
   *      error to open a file that does not exist for read-only operations.
   * 'w': opens for reading and writing, but truncates the file if it
   *      exists; it is not an error to open files that do not exist with
   *      this flag. 
   * 'a': opens for reading and writing - any type of modification can 
   *      occur. If the file does not exist, this flag is effectively like
   *      'w'.
   *
   * Returns a newly allocated File object that can read and write data to the
   * file using a specific backend.
   */
  typedef boost::shared_ptr<File> (*file_factory_t)
    (const std::string& filename, char mode);

}}

#endif /* TORCH_IO_FILE_H */
