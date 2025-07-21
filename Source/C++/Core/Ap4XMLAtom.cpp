/*****************************************************************
|
|    AP4 - xml Atom
|    
|    Tsviatko Jongov - jongov@gmail.com
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4XMLAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <pugixml.hpp>

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_XMLAtom)

/*----------------------------------------------------------------------
|   AP4_XMLAtom::Create
+---------------------------------------------------------------------*/
AP4_XMLAtom*
AP4_XMLAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version != 0) return NULL;
    return new AP4_XMLAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_XMLAtom::AP4_XMLAtom
+---------------------------------------------------------------------*/
AP4_XMLAtom::AP4_XMLAtom(const char* xml) :
    AP4_Atom(AP4_ATOM_TYPE_XML, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{

}
// Function to recursively convert XML to a JSON-like structure
std::string xmlToJson(const pugi::xml_node& node, int indent = 0) {
    std::ostringstream result;

    // Indentation for formatting
    std::string indentation(indent, ' ');

    result << "{\n";

    // Process attributes
    for (const auto& attr : node.attributes()) {
        result << indentation << "  \"" << attr.name() << "\": \"" << attr.value() << "\",\n";
    }

    // Process child nodes
    bool hasChild = false;
    for (const auto& child : node.children()) {
        if (child.type() == pugi::node_element) {
            if (!hasChild) {
                result << "\n";
                hasChild = true;
            }
            result << indentation << "  \"" << child.name() << "\": ";
            result << xmlToJson(child, indent + 2) << ",\n";
        }
        else if (child.type() == pugi::node_pcdata) {
            // Text content
            if (!hasChild) {
                result << "\n";
                hasChild = true;
            }
            result << indentation << "  \"text\": \"" << child.value() << "\",\n";
        }
    }

    if (hasChild) {
        // Remove trailing comma
        result.seekp(-2, std::ios_base::end);
        result << "\n" << indentation;
    }

    result << "}";

    return result.str();
}

/*----------------------------------------------------------------------
|   AP4_XMLAtom::AP4_XMLAtom
+---------------------------------------------------------------------*/
AP4_XMLAtom::AP4_XMLAtom(AP4_UI32        size, 
                         AP4_UI08        version,
                         AP4_UI32        flags,
                         AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_XML, size, version, flags)
{    
    // read the name unless it is empty
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return;
    AP4_UI32 name_size = size-(AP4_FULL_ATOM_HEADER_SIZE);
    char* name = new char[name_size+1];

    if (name == NULL) return;

    stream.Read(name, name_size);

    name[name_size] = '\0'; // force a null termination    

    // Load XML from the char* variable
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(name, strlen(name));

    if (result) {
        // Convert XML to JSON-like structure
        std::string jsonString = xmlToJson(doc.first_child());

        m_JSON = jsonString.c_str();
    }

    // handle a special case: the Quicktime files have a pascal
    // string here, but ISO MP4 files have a C string.
    // we try to detect a pascal encoding and correct it.
    if ((AP4_UI08)name[0] == (AP4_UI08)(name_size-1)) {
        m_XML = name+1;                
    } else {
        m_XML = name;
    }
    delete[] name;


}

/*----------------------------------------------------------------------
|   AP4_XMLAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_XMLAtom::WriteFields(AP4_ByteStream& stream)
{
    return AP4_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   AP4_XMLAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_XMLAtom::InspectFields(AP4_AtomInspector& inspector)
{
    //Return XML
    inspector.AddField("xml", m_XML.GetChars());

    //Return JSON
    //inspector.AddField("xml", m_JSON.GetChars());

    return AP4_SUCCESS;
}
