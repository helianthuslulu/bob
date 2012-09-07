/**
 * @file bob/core/python/container.h
 * @date Tue Jun 21 17:28:25 2011 +0200
 * @author Andre Anjos <andre.anjos@idiap.ch>
 *
 * @brief Generic container conversions for boost::python, extracted from: http://cctbx.cvs.sourceforge.net/viewvc/cctbx/scitbx/include/scitbx/boost_python/container_conversions.h
 * version 1.16 (not touched for at least 5 years).
 * How to bind your container using this code:
 * 1) to bind a std::vector use variable_capacity_policy:
 * from_python_sequence<std::vector<double, 3>, variable_capacity_policy>();
 * 2) to bind a std::list use linked_list_policy:
 * from_python_sequence<std::list<double, 3>, linked_list_policy>();
 * 3) to bind a container that has fixed capacity, use fixed_size_policy:
 * from_python_sequence<boost::array<double,3>, fixed_size_policy>();
 *
 * Copyright (C) 2011-2012 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOB_CORE_PYTHON_CONTAINER_H 
#define BOB_CORE_PYTHON_CONTAINER_H

#include <boost/python.hpp>

namespace bob { namespace python {

  template <typename ContainerType>
  struct to_tuple
  {
    static PyObject* convert(ContainerType const& a)
    {
      boost::python::list result;
      typedef typename ContainerType::const_iterator const_iter;
      for(const_iter p=a.begin();p!=a.end();p++) {
        result.append(boost::python::object(*p));
      }
      return boost::python::incref(boost::python::tuple(result).ptr());
    }
  };

  struct default_policy
  {
    static bool check_convertibility_per_element() { return false; }

    template <typename ContainerType>
    static bool check_size(boost::type<ContainerType>, std::size_t /*sz*/)
    {
      return true;
    }

    template <typename ContainerType>
    static void assert_size(boost::type<ContainerType>, std::size_t /*sz*/) {}

    template <typename ContainerType>
    static void reserve(ContainerType& a, std::size_t sz) {}
  };

  struct fixed_size_policy
  {
    static bool check_convertibility_per_element() { return true; }

    template <typename ContainerType>
    static bool check_size(boost::type<ContainerType>, std::size_t sz)
    {
      return ContainerType::size() == sz;
    }

    template <typename ContainerType>
    static void assert_size(boost::type<ContainerType>, std::size_t sz)
    {
      if (!check_size(boost::type<ContainerType>(), sz)) {
        PyErr_SetString(PyExc_RuntimeError,
          "Insufficient elements for fixed-size array.");
        boost::python::throw_error_already_set();
      }
    }

    template <typename ContainerType>
    static void reserve(ContainerType& /*a*/, std::size_t sz)
    {
      if (sz > ContainerType::size()) {
        PyErr_SetString(PyExc_RuntimeError,
          "Too many elements for fixed-size array.");
        boost::python::throw_error_already_set();
      }
    }

    template <typename ContainerType, typename ValueType>
    static void set_value(ContainerType& a, std::size_t i, ValueType const& v)
    {
      reserve(a, i+1);
      a[i] = v;
    }
  };

  struct variable_capacity_policy : default_policy
  {
    static bool check_convertibility_per_element() { return true; }

    template <typename ContainerType>
    static void reserve(ContainerType& a, std::size_t sz)
    {
      a.reserve(sz);
    }

    template <typename ContainerType, typename ValueType>
    static void set_value(
      ContainerType& a,
      std::size_t
#if !defined(NDEBUG)
      i
#endif
      ,
      ValueType const& v)
    {
      assert(a.size() == i);
      a.push_back(v);
    }
  };

  struct fixed_capacity_policy : variable_capacity_policy
  {
    template <typename ContainerType>
    static bool check_size(boost::type<ContainerType>, std::size_t sz)
    {
      return ContainerType::max_size() >= sz;
    }
  };

  struct linked_list_policy : default_policy
  {
    template <typename ContainerType, typename ValueType>
    static void
    set_value(ContainerType& a, std::size_t /*i*/, ValueType const& v)
    {
      a.push_back(v);
    }
  };

  struct set_policy : default_policy
  {
    template <typename ContainerType, typename ValueType>
    static void
    set_value(ContainerType& a, std::size_t /*i*/, ValueType const& v)
    {
      a.insert(v);
    }
  };

  template <typename ContainerType, typename ConversionPolicy>
  struct from_python_sequence
  {
    typedef typename ContainerType::value_type container_element_type;

    from_python_sequence()
    {
      boost::python::converter::registry::push_back(
        &convertible,
        &construct,
        boost::python::type_id<ContainerType>());
    }

    static void* convertible(PyObject* obj_ptr)
    {
      if (!(   PyList_Check(obj_ptr)
            || PyTuple_Check(obj_ptr)
            || PyIter_Check(obj_ptr)
            || PyRange_Check(obj_ptr)
            || (   !PyString_Check(obj_ptr)
                && !PyUnicode_Check(obj_ptr)
                && (   obj_ptr->ob_type == 0
                    || obj_ptr->ob_type->ob_type == 0
                    || obj_ptr->ob_type->ob_type->tp_name == 0
                    || std::strcmp(
                         obj_ptr->ob_type->ob_type->tp_name,
                         "Boost.Python.class") != 0)
                && PyObject_HasAttrString(obj_ptr, "__len__")
                && PyObject_HasAttrString(obj_ptr, "__getitem__")))) return 0;
      boost::python::handle<> obj_iter(
        boost::python::allow_null(PyObject_GetIter(obj_ptr)));
      if (!obj_iter.get()) { // must be convertible to an iterator
        PyErr_Clear();
        return 0;
      }
      if (ConversionPolicy::check_convertibility_per_element()) {
        int obj_size = PyObject_Length(obj_ptr);
        if (obj_size < 0) { // must be a measurable sequence
          PyErr_Clear();
          return 0;
        }
        if (!ConversionPolicy::check_size(
          boost::type<ContainerType>(), obj_size)) return 0;
        bool is_range = PyRange_Check(obj_ptr);
        std::size_t i=0;
        if (!all_elements_convertible(obj_iter, is_range, i)) return 0;
        if (!is_range) assert(i == (size_t)obj_size);
      }
      return obj_ptr;
    }

    // This loop factored out by Achim Domma to avoid Visual C++
    // Internal Compiler Error.
    static bool
    all_elements_convertible(
      boost::python::handle<>& obj_iter,
      bool is_range,
      std::size_t& i)
    {
      for(;;i++) {
        boost::python::handle<> py_elem_hdl(
          boost::python::allow_null(PyIter_Next(obj_iter.get())));
        if (PyErr_Occurred()) {
          PyErr_Clear();
          return false;
        }
        if (!py_elem_hdl.get()) break; // end of iteration
        boost::python::object py_elem_obj(py_elem_hdl);
        boost::python::extract<container_element_type>
          elem_proxy(py_elem_obj);
        if (!elem_proxy.check()) return false;
        if (is_range) break; // in a range all elements are of the same type
      }
      return true;
    }

    static void construct(
      PyObject* obj_ptr,
      boost::python::converter::rvalue_from_python_stage1_data* data)
    {
      boost::python::handle<> obj_iter(PyObject_GetIter(obj_ptr));
      void* storage = (
        (boost::python::converter::rvalue_from_python_storage<ContainerType>*)
          data)->storage.bytes;
      new (storage) ContainerType();
      data->convertible = storage;
      ContainerType& result = *((ContainerType*)storage);
      std::size_t i=0;
      for(;;i++) {
        boost::python::handle<> py_elem_hdl(
          boost::python::allow_null(PyIter_Next(obj_iter.get())));
        if (PyErr_Occurred()) boost::python::throw_error_already_set();
        if (!py_elem_hdl.get()) break; // end of iteration
        boost::python::object py_elem_obj(py_elem_hdl);
        boost::python::extract<container_element_type> elem_proxy(py_elem_obj);
        ConversionPolicy::set_value(result, i, elem_proxy());
      }
      ConversionPolicy::assert_size(boost::type<ContainerType>(), i);
    }
  };

  template <typename ContainerType>
  struct to_tuple_mapping
  {
    to_tuple_mapping() {
      boost::python::to_python_converter<
        ContainerType,
        to_tuple<ContainerType> >();
    }
  };

  template <typename ContainerType, typename ConversionPolicy>
  struct tuple_mapping : to_tuple_mapping<ContainerType>
  {
    tuple_mapping() {
      from_python_sequence<
        ContainerType,
        ConversionPolicy>();
    }
  };

  template <typename ContainerType>
  struct tuple_mapping_fixed_size
  {
    tuple_mapping_fixed_size() {
      tuple_mapping<
        ContainerType,
        fixed_size_policy>();
    }
  };

  template <typename ContainerType>
  struct tuple_mapping_fixed_capacity
  {
    tuple_mapping_fixed_capacity() {
      tuple_mapping<
        ContainerType,
        fixed_capacity_policy>();
    }
  };

  template <typename ContainerType>
  struct tuple_mapping_variable_capacity
  {
    tuple_mapping_variable_capacity() {
      tuple_mapping<
        ContainerType,
        variable_capacity_policy>();
    }
  };

  template <typename ContainerType>
  struct tuple_mapping_set
  {
    tuple_mapping_set() {
      tuple_mapping<
        ContainerType,
        set_policy>();
    }
  };

}} // namespace bob::python

#endif /* BOB_CORE_PYTHON_CONTAINER_H */