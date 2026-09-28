class MockXML_FILE {
  public:
    MOCK_METHOD1(fileio_open, bool(const char* path));
    MOCK_METHOD1(fileio_get_find_node, xmlNodePtr(const char* beginning_key));
    MOCK_CONST_METHOD1(fileio_get_prop, xmlChar*(const char* key));
//     MOCK_METHOD2(fileio_set_prop, void(const char* key, const char* value));
    MOCK_METHOD1(fileio_get_child_data, xmlChar*(const char* key));
    MOCK_CONST_METHOD2(fileio_get_child_data, xmlChar*(const char* key, const char* attribute));
//     MOCK_METHOD1(fileio_find_next_node, void(const char* key));
    MOCK_METHOD2(fileio_find_next_node, xmlNodePtr(xmlNodePtr input_ptr, const char* key));
//     MOCK_METHOD1(set_command, void(const char* key));
//     MOCK_METHOD1(fileio_free, void(xmlChar* str));
    MOCK_METHOD2(fileio_create, bool(const char* const file_name , const char* const file_version));
    MOCK_METHOD2(fileio_add_start_element, bool(xmlTextWriterPtr text_ptr, const char* name));
    MOCK_METHOD3(fileio_add_element, bool(xmlTextWriterPtr text_ptr, const char* name, const char* value));
    MOCK_METHOD1(fileio_add_end_element, bool(xmlTextWriterPtr text_ptr));
    MOCK_METHOD3(fileio_add_attribute, bool(xmlTextWriterPtr text_ptr, const char* name, const char* value));
    MOCK_METHOD1(fileio_create_end, bool(xmlTextWriterPtr text_ptr));
    MOCK_METHOD2(fileio_find_tree, xmlNodePtr(xmlNodePtr p_cur, const xmlChar* find));
};

MockXML_FILE * M_XML_FILE;

XML_FILE::XML_FILE()
{

}

XML_FILE::~XML_FILE()
{

}

bool XML_FILE::fileio_open(const char* path)
{
    return M_XML_FILE->fileio_open(path);
}

xmlNodePtr XML_FILE::fileio_get_find_node(const char* beginning_key)
{
    return M_XML_FILE->fileio_get_find_node(beginning_key);
}

xmlChar* XML_FILE::fileio_get_prop(const char* key) const
{
    return M_XML_FILE->fileio_get_prop(key);
}

void XML_FILE::fileio_set_prop(const char* key, const char* value)
{
//    M_XML_FILE->fileio_set_prop(key, value);
}

xmlChar* XML_FILE::fileio_get_child_data(const char* key)
{
    return M_XML_FILE->fileio_get_child_data(key);
}

xmlChar* XML_FILE::fileio_get_child_data(const char* key, const char* attribute) const
{
    return M_XML_FILE->fileio_get_child_data(key, attribute);
}

void XML_FILE::fileio_find_next_node(const char* key)
{
//    M_XML_FILE->fileio_find_next_node(key);
}

xmlNodePtr XML_FILE::fileio_find_next_node(xmlNodePtr input_ptr, const char* key)
{
    return M_XML_FILE->fileio_find_next_node(input_ptr, key);
}

void XML_FILE::set_command(const char* key)
{
//    M_XML_FILE->set_command(key);
}

void XML_FILE::fileio_free(xmlChar* str)
{
//    M_XML_FILE->fileio_free(str);
}

bool XML_FILE::fileio_create(const char* const file_name , const char* const file_version)
{
    return M_XML_FILE->fileio_create(file_name, file_version);
}

bool XML_FILE::fileio_add_start_element(xmlTextWriterPtr text_ptr, const char* name)
{
    return M_XML_FILE->fileio_add_start_element(text_ptr, name);
}

bool XML_FILE::fileio_add_element(xmlTextWriterPtr text_ptr, const char* name, const char* value)
{
    return M_XML_FILE->fileio_add_element(text_ptr, name, value);
}

bool XML_FILE::fileio_add_end_element(xmlTextWriterPtr text_ptr)
{
    return M_XML_FILE->fileio_add_end_element(text_ptr);
}

bool XML_FILE::fileio_add_attribute(xmlTextWriterPtr text_ptr, const char* name, const char* value)
{
    return M_XML_FILE->fileio_add_attribute(text_ptr, name, value);
}

bool XML_FILE::fileio_create_end(xmlTextWriterPtr text_ptr)
{
    return M_XML_FILE->fileio_create_end(text_ptr);
}

xmlNodePtr XML_FILE::fileio_find_tree(xmlNodePtr p_cur, const xmlChar* find)
{
    return M_XML_FILE->fileio_find_tree(p_cur, find);
}
