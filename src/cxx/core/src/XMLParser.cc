/**
 * @file src/cxx/core/src/XMLParser.cc
 * @author <a href="mailto:Laurent.El-Shafey@idiap.ch">Laurent El Shafey</a>
 *
 * @brief Implements the XML parser for a dataset.
 */

#include "core/XMLParser.h"
#include "core/Exception.h"

#include <libxml/xmlschemas.h>

namespace Torch {
  namespace core {

    XMLParser::XMLParser(): m_id_role( new std::map<size_t,std::string>() )
    {
    }

    XMLParser::~XMLParser() { }


    void XMLParser::validateXMLSchema(xmlDocPtr doc) {
      // Get path to the XML Schema definition
      char *schema_path = getenv("TORCH_SCHEMA_PATH");
      if( !strcmp( schema_path, "") )
        warn << "Environment variable $TORCH_SCHEMA_PATH is not set." <<
          std::endl;
      char schema_full_path[1024];
      strcpy( schema_full_path, schema_path);
      strcat( schema_full_path, "/dataset.xsd" );

      // Load the XML schema from the file
      xmlDocPtr xsd_doc = xmlReadFile(schema_full_path, 0, 0);
      if(xsd_doc == 0) {
        error << "Unable to load the XML schema" << std::endl;
        throw Exception();        
      }
      // Create an XML schema parse context
      xmlSchemaParserCtxtPtr xsd_parser = xmlSchemaNewDocParserCtxt(xsd_doc);
      if(xsd_parser == 0) {
        xmlFreeDoc(xsd_doc);
        error << "Unable to create the XML schema parse context." << std::endl;
        throw Exception();        
      }
      // Parse the XML schema definition and check its correctness
      xmlSchemaPtr xsd_schema = xmlSchemaParse(xsd_parser);
      if(xsd_schema == 0) {
        xmlSchemaFreeParserCtxt(xsd_parser);
        xmlFreeDoc(xsd_doc); 
        error << "Invalid XML Schema definition." << std::endl;
        throw Exception();        
      }
      // Create an XML schema validation context for the schema
      xmlSchemaValidCtxtPtr xsd_valid = xmlSchemaNewValidCtxt(xsd_schema);
      if(xsd_valid == 0) {
        xmlSchemaFree(xsd_schema);
        xmlSchemaFreeParserCtxt(xsd_parser);
        xmlFreeDoc(xsd_doc);
        error << "Unable to create an XML Schema validation context." <<
          std::endl;
        throw Exception();
      }

      // Check that the document is valid wrt. to the schema, and throw an 
      // exception otherwise.
      if(xmlSchemaValidateDoc(xsd_valid, doc)) {
        error << "The XML file is NOT valid with respect to the XML schema." <<
          std::endl;
        throw Exception();
      }

      xmlSchemaFreeValidCtxt(xsd_valid);
      xmlSchemaFree(xsd_schema);
      xmlSchemaFreeParserCtxt(xsd_parser);
      xmlFreeDoc(xsd_doc);
    }


    void XMLParser::load(const char* filename, Dataset& dataset, 
      size_t check_level) 
    {
      // Parse the XML file with libxml2
      xmlDocPtr doc = xmlParseFile(filename);
      xmlNodePtr cur; 

      // Check validity of the XML file
      if(doc == 0 ) {
        error << "Document " << filename << " was not parsed successfully." <<
          std::endl;
        throw Exception();
      }

      // Check that the XML file is not empty
      cur = xmlDocGetRootElement(doc);
      if(cur == 0) { 
        error << "Document " << filename << " is empty." << std::endl;
        xmlFreeDoc(doc);
        throw Exception();
      }

      // Check that the XML file contains a dataset
      if( strcmp((const char*)cur->name, db::dataset) ) {
        error << "Document " << filename << 
          " is of the wrong type (!= dataset)." << std::endl;
        xmlFreeDoc(doc);
        throw Exception();
      } 

      // Validate the XML document against the XML Schema
      // Throw an exception in case of failure
      validateXMLSchema( doc);

      // Parse Dataset Attributes
      // 1/ Parse name
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::name);
      dataset.setName( ( (str!=0?(const char *)str:"") ) );
      TDEBUG3("Name: " << dataset.getName());
      xmlFree(str);

      // 2/ Parse version 
      str = xmlGetProp(cur, (const xmlChar*)db::version);
      dataset.setVersion( str!=0 ? 
        boost::lexical_cast<size_t>((const char *)str) : 0 );
      TDEBUG3("Version: " << dataset.getVersion());
      xmlFree(str);

      // 3/ Parse date
      // TODO: implementation
      

      // Parse Arraysets and Relationsets
      cur = cur->xmlChildrenNode;
      while(cur != 0) { 
        // Parse an arrayset and add it to the dataset
        if( !strcmp((const char*)cur->name, db::arrayset) || 
            !strcmp((const char*)cur->name, db::external_arrayset) )
          dataset.addArrayset( parseArrayset(cur) );
        // Parse a relationset and add it to the dataset
        else if( !strcmp((const char*)cur->name, db::relationset) )
          dataset.addRelationset( parseRelationset(cur) );
        cur = cur->next;
      }

      
      // High-level checks (which can not be done by libxml2)
      if( check_level>= 1)
      {
        TDEBUG3(std::endl << "HIGH-LEVEL CHECKS");
        // Iterate over the relationsets
        for( Dataset::relationset_const_iterator 
          relationset = dataset.relationset_begin();
          relationset != dataset.relationset_end(); ++relationset )
        {
          TDEBUG3("Relationset name: " << relationset->second->getName()); 

          // Check that the rules are correct.
          //   (arrayset-role refers to an existing string role)
          for( Relationset::rule_const_iterator 
            rule = relationset->second->rule_begin();
            rule != relationset->second->rule_end(); ++rule )
          {
            TDEBUG3("Rule role: " << rule->second->getArraysetRole()); 
            bool found = false;
            for( Dataset::const_iterator arrayset = dataset.begin(); 
              arrayset != dataset.end(); ++arrayset )
            {
              if( !rule->second->getArraysetRole().compare( 
                arrayset->second->getRole() ) )
              {
                found = true;
                break;
              }
            }

            if( !found ) {
              error << "Rule refers to a non-existing arrayset-role (" << 
                rule->second->getArraysetRole() << ")." << std::endl;
              throw Exception();
            }
          }

          // Check that the relations are correct
          for( Relationset::const_iterator 
            relation = relationset->second->begin();
            relation != relationset->second->end(); ++relation )
          {
            TDEBUG3("Relation id: " << relation->second->getId()); 

            // Check that for each rule in the relationset, the multiplicity
            // of the members is correct.
            for( Relationset::rule_const_iterator 
              rule = relationset->second->rule_begin();
              rule != relationset->second->rule_end(); ++rule )
            {
              TDEBUG3("Rule id: " << rule->second->getArraysetRole());

              size_t counter = 0;
              bool check_ok = true;
              for( Relation::const_iterator member = relation->second->begin();
                member != relation->second->end(); ++member )
              {
                TDEBUG3("  Member ids: " << member->second->getArrayId()
                  << "," << member->second->getArraysetId());
                TDEBUG3("  " << (*m_id_role)[member->second->getArraysetId()]);
                TDEBUG3("  " << rule->second->getArraysetRole() << std::endl);

                if( !(*m_id_role)[member->second->getArraysetId()].compare( 
                  rule->second->getArraysetRole() ) )
                {
                  TDEBUG3("  Array id: " << member->second->getArrayId());
                  if( member->second->getArrayId()!=0 )
                    ++counter;
                  else // Arrayset-member
                  {
                    const Arrayset &ar = 
                      dataset[member->second->getArraysetId()];
                    if( ar.getIsLoaded() )
                      counter += ar.getNArrays();
                    else if( check_level >= 2 ) {
                      ;//TODO: load the arrayset
                      // counter += ar.getNArrays();
                    }
                    else
                      check_ok = false;
                  }
                }
              }

              TDEBUG3("  Counter: " << counter);
              if( check_ok && ( counter<rule->second->getMin() || 
                (rule->second->getMax()!=0 && counter>rule->second->getMax()) ) )
              {
                error << "Relation (id=" << relation->second->getId() << 
                  ") is not valid." << std::endl;
                throw Exception();
              }
              else if(!check_ok)
                warn << "Relation (id=" << relation->second->getId() <<
                  ") has not been fully checked, because of external data." << 
                  std::endl;
            }

            // Check that there is no member referring to a non-existing rule.
            for( Relation::const_iterator member = relation->second->begin();
              member != relation->second->end(); ++member )
            {
              TDEBUG3("  Member ids: " << member->second->getArrayId() <<
                "," << member->second->getArraysetId());

              bool found = false;
              for( Relationset::rule_const_iterator 
                rule = relationset->second->rule_begin();
                rule != relationset->second->rule_end(); ++rule )
              {
                TDEBUG3("Rule id: " << rule->second->getArraysetRole()); 
                if( !(*m_id_role)[member->second->getArraysetId()].compare(
                  rule->second->getArraysetRole() ) )
                {
                  found = true;
                  break;
                }
              }

              if( !found ) {
                error << "Member (id:" << member->second->getArrayId() <<
                  "," << member->second->getArraysetId() << 
                  ") refers to a non-existing rule." << std::endl;
                throw Exception();
              }
            }
          }
        }
      }

      xmlFreeDoc(doc);
    }


    boost::shared_ptr<Relationset> 
    XMLParser::parseRelationset(const xmlNodePtr cur) 
    {
      boost::shared_ptr<Relationset> relationset(new Relationset());
      // Parse name
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::name);
      relationset->setName( ( (str!=0?(const char *)str:"") ) );
      TDEBUG3("Name: " << relationset->getName());
      xmlFree(str);
      
      // Parse the relations and rules
      xmlNodePtr cur_relation = cur->xmlChildrenNode;
      while(cur_relation != 0) { 
        // Parse a rule and add it to the relationset
        if( !strcmp((const char*)cur_relation->name, db::rule) ) 
          relationset->addRule( parseRule(cur_relation) );
        // Parse a relation and add it to the relationset
        else if( !strcmp((const char*)cur_relation->name, db::relation) ) 
          relationset->addRelation( parseRelation(cur_relation) );
        cur_relation = cur_relation->next;
      }

      return relationset;
    }


    boost::shared_ptr<Rule> XMLParser::parseRule(const xmlNodePtr cur) {
      boost::shared_ptr<Rule> rule(new Rule());
      // Parse arrayset-role
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::arrayset_role);
      rule->setArraysetRole( ( (str!=0?(const char *)str:"") ) );
      TDEBUG3("  Arrayset-role: " << rule->getArraysetRole());
      xmlFree(str);
      
      // Parse min
      str = xmlGetProp(cur, (const xmlChar*)db::min);
      rule->setMin(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("  Min: " << rule->getMin());
      xmlFree(str);

      // Parse max
      str = xmlGetProp(cur, (const xmlChar*)db::max);
      rule->setMax(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("  Max: " << rule->getMax());
      xmlFree(str);

      return rule;
    }


    boost::shared_ptr<Relation> XMLParser::parseRelation(const xmlNodePtr cur) {
      boost::shared_ptr<Relation> relation(new Relation(m_id_role));
      // Parse id
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::id);
      relation->setId(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("  Id: " << relation->getId());
      xmlFree(str);

      // Parse the members
      xmlNodePtr cur_member = cur->xmlChildrenNode;
      while(cur_member != 0) { 
        // Parse a member and add it to the relation
        if( !strcmp((const char*)cur_member->name, db::member) ||
          !strcmp((const char*)cur_member->name, db::arrayset_member) ) 
          relation->addMember( parseMember(cur_member) );
        cur_member = cur_member->next;
      }

      return relation;
    }


    boost::shared_ptr<Member> XMLParser::parseMember(const xmlNodePtr cur) {
      boost::shared_ptr<Member> member(new Member());
      // Parse array-id
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::array_id);
      member->setArrayId(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("    Array-id: " << member->getArrayId());
      xmlFree(str);

      // Parse arrayset-id
      str = xmlGetProp(cur, (const xmlChar*)db::arrayset_id);
      member->setArraysetId(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("    Arrayset-id: " << member->getArraysetId());
      xmlFree(str);

      return member;
    }


    boost::shared_ptr<Arrayset> XMLParser::parseArrayset(const xmlNodePtr cur) {
      boost::shared_ptr<Arrayset> arrayset(new Arrayset());
      // Parse id
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::id);
      arrayset->setId(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("Id: " << arrayset->getId());
      xmlFree(str);

      // Parse role
      str = xmlGetProp(cur, (const xmlChar*)db::role);
      arrayset->setRole( ( (str!=0?(const char *)str:"") ) );
      TDEBUG3("Role: " << arrayset->getRole());
      xmlFree(str);

      // Add id-role to the mapping of the XMLParser. This will be used for
      // checking the members of a relation.
      m_id_role->insert( std::pair<size_t,std::string>( 
        arrayset->getId(), arrayset->getRole()) );

      // Parse elementtype
      str = xmlGetProp(cur, (const xmlChar*)db::elementtype);
      if( str==0 ) {
        error << "Elementtype is not specified in arrayset (id: " << 
          arrayset->getId() << ")." << std::endl;
        throw Exception();
      }
      std::string str_element_type( (const char*)str );
      if( !str_element_type.compare( db::t_bool ) )
        arrayset->setArrayType( array::t_bool );
      else if( !str_element_type.compare( db::t_uint8 ) )
        arrayset->setArrayType( array::t_uint8 );
      else if( !str_element_type.compare( db::t_uint16 ) )
        arrayset->setArrayType( array::t_uint16 );
      else if( !str_element_type.compare( db::t_uint32 ) )
        arrayset->setArrayType( array::t_uint32 );
      else if( !str_element_type.compare( db::t_uint64 ) )
        arrayset->setArrayType( array::t_uint64 );
      else if( !str_element_type.compare( db::t_int8 ) )
        arrayset->setArrayType( array::t_int8 );
      else if( !str_element_type.compare( db::t_int16 ) )
        arrayset->setArrayType( array::t_int16 );
      else if( !str_element_type.compare( db::t_int32 ) )
        arrayset->setArrayType( array::t_int32 );
      else if( !str_element_type.compare( db::t_int64 ) )
        arrayset->setArrayType( array::t_int64 );
      else if( !str_element_type.compare( db::t_float32 ) )
        arrayset->setArrayType( array::t_float32 );
      else if( !str_element_type.compare( db::t_float64 ) )
        arrayset->setArrayType( array::t_float64 );
      else if( !str_element_type.compare( db::t_float128 ) )
        arrayset->setArrayType( array::t_float128 );
      else if( !str_element_type.compare( db::t_complex64 ) )
        arrayset->setArrayType( array::t_complex64 );
      else if( !str_element_type.compare( db::t_complex128 ) )
        arrayset->setArrayType( array::t_complex128 );
      else if( !str_element_type.compare( db::t_complex256 ) )
        arrayset->setArrayType( array::t_complex256 );
      else
        arrayset->setArrayType( array::t_unknown );
      TDEBUG3("Elementtype: " << arrayset->getArrayType());
      xmlFree(str);

      // Parse shape
      size_t shape[array::N_MAX_DIMENSIONS_ARRAY];
      for(size_t i=0; i<array::N_MAX_DIMENSIONS_ARRAY; ++i)
        shape[i]=0;
      str = xmlGetProp(cur, (const xmlChar*)db::shape);
      if( str==0 ) {
        error << "Elementtype is not specified in arrayset (id: " << 
          arrayset->getId() << ")." << std::endl;
        throw Exception();
      }
      // Tokenize the shape string to extract the dimensions
      std::string str_shape((const char *)str);
      boost::tokenizer<> tok(str_shape);
      size_t count=0;
      for( boost::tokenizer<>::iterator it=tok.begin(); it!=tok.end(); 
        ++it, ++count ) 
      {
        if(count>=array::N_MAX_DIMENSIONS_ARRAY) {
          error << "Shape is not valid in arrayset (id: " << 
            arrayset->getId() << "). Maximum number of dimensions is " <<
            array::N_MAX_DIMENSIONS_ARRAY << "." << std::endl;
          throw Exception();        
        }
        shape[count] = boost::lexical_cast<size_t>((*it).c_str());
      }
      arrayset->setNDim(count);
      arrayset->setShape(shape);
      TDEBUG3("Nb dimensions: " << arrayset->getNDim());
      TDEBUG3("Shape: (" << arrayset->getShape()[0] << "," << 
        arrayset->getShape()[1] << ","<< arrayset->getShape()[2] << "," << 
        arrayset->getShape()[3] << ")");
      xmlFree(str);
      // Set the number of elements
      size_t n_elem = arrayset->getShape()[0];
      for( size_t i=1; i < arrayset->getNDim(); ++i)
        n_elem *= arrayset->getShape()[i];
      arrayset->setNElem(n_elem);

      // Parse loader
      str = xmlGetProp(cur, (const xmlChar*)db::loader);
      std::string str_loader( str!=0 ? (const char*)str: "" );
      if( !str_loader.compare( db::l_blitz ) )
        arrayset->setLoader( l_blitz );
      else if( !str_loader.compare( db::l_tensor ) )
        arrayset->setLoader( l_tensor );
      else if( !str_loader.compare( db::l_bindata ) )
        arrayset->setLoader( l_bindata );
      else 
        arrayset->setLoader( l_unknown );
      TDEBUG3("Loader: " << arrayset->getLoader());
      xmlFree(str);

      // Parse filename
      str = xmlGetProp(cur, (const xmlChar*)db::file);
      arrayset->setFilename( (str!=0?(const char*)str:"") );
      TDEBUG3("File: " << arrayset->getFilename());
      xmlFree(str);

      if( !arrayset->getFilename().compare("") )
      {
        // Parse the data
        xmlNodePtr cur_data = cur->xmlChildrenNode;

        while (cur_data != 0) { 
          // Process an array
          if ( !strcmp( (const char*)cur_data->name, db::array) || 
               !strcmp( (const char*)cur_data->name, db::external_array) ) {
            arrayset->addArray( parseArray( *arrayset, cur_data) );
          }
          cur_data = cur_data->next;
        }

        arrayset->setIsLoaded(true);
      }

      return arrayset;
    }


    boost::shared_ptr<Array> XMLParser::parseArray(
      const Arrayset& parent, const xmlNodePtr cur) 
    {
      boost::shared_ptr<Array> array(new Array(parent));
      // Parse id
      xmlChar *str;
      str = xmlGetProp(cur, (const xmlChar*)db::id);
      array->setId(str!=0? boost::lexical_cast<size_t>((const char*)str): 0);
      TDEBUG3("  Array Id: " << array->getId());
      xmlFree(str);

      // Parse loader
      str = xmlGetProp(cur, (const xmlChar*)db::loader);
      std::string str_loader( str!=0 ? (const char*)str: "" );
      if( !str_loader.compare( db::l_blitz ) )
        array->setLoader( l_blitz );
      else if( !str_loader.compare( db::l_tensor ) )
        array->setLoader( l_tensor );
      else if( !str_loader.compare( db::l_bindata ) )
        array->setLoader( l_bindata );
      else 
        array->setLoader( l_unknown );
      TDEBUG3("  Array Loader: " << array->getLoader());
      xmlFree(str);

      // Parse filename
      str = xmlGetProp(cur, (const xmlChar*)db::file);
      array->setFilename( (str!=0?(const char*)str:"") );
      TDEBUG3("  Array File: " << array->getFilename());
      xmlFree(str);

      // Parse the data contained in the XML file
      if( !array->getFilename().compare("") )
      {
        // Preliminary for the processing of the content of the array
        xmlChar* content = xmlNodeGetContent(cur);
        std::string data( (const char *)content);
        boost::char_separator<char> sep(" ;|");
        boost::tokenizer<boost::char_separator<char> > tok(data, sep);
        xmlFree(content);

        // Switch over the possible type
        size_t nb_values = parent.getNElem();
        switch( parent.getArrayType()) {
          case array::t_bool:
            array->setStorage( parseArrayData<bool>( tok, nb_values ) );
            break;
          case array::t_int8:
            array->setStorage( parseArrayData<int8_t>( tok, nb_values ) );
            break;
          case array::t_int16:
            array->setStorage( parseArrayData<int16_t>( tok, nb_values ) );
            break;
          case array::t_int32:
            array->setStorage( parseArrayData<int32_t>( tok, nb_values ) );
            break;
          case array::t_int64:
            array->setStorage( parseArrayData<int64_t>( tok, nb_values ) );
            break;
          case array::t_uint8:
            array->setStorage( parseArrayData<uint8_t>( tok, nb_values ) );
            break;
          case array::t_uint16:
            array->setStorage( parseArrayData<uint16_t>( tok, nb_values ) );
            break;
          case array::t_uint32:
            array->setStorage( parseArrayData<uint32_t>( tok, nb_values ) );
            break;
          case array::t_uint64:
            array->setStorage( parseArrayData<uint64_t>( tok, nb_values ) );
            break;
          case array::t_float32:
            array->setStorage( parseArrayData<float>( tok, nb_values ) );
            break;
          case array::t_float64:
            array->setStorage( parseArrayData<double>( tok, nb_values ) );
            break;
          case array::t_float128:
            array->setStorage( parseArrayData<long double>( tok, nb_values ) );
            break;
          case array::t_complex64:
            array->setStorage( parseArrayData<std::complex<float> >( tok, 
              nb_values ) );
            break;
          case array::t_complex128:
            array->setStorage( parseArrayData<std::complex<double> >( tok, 
              nb_values ) );
            break;
          case array::t_complex256:
            array->setStorage( parseArrayData<std::complex<long double> >( 
              tok, nb_values ) );
            break;
          default:
            break;
        }

        array->setIsLoaded(true);
      }
      
      return array;
    }


  }
}
