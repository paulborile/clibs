#include <gtest/gtest.h>


#include "fh.h"

#include "CommandLineParams.h"

#include <fstream>
using namespace std;

struct dataload
{
    int numval;
    char textval[10];
};

struct dataobject
{
    char *keyval;
    int numval;
    char textval[10];
};

string ToString(int val)
{
    stringstream stream;
    stream << val;
    return stream.str();
}

string uno = "Primo elemento";
string due = "Secondo elemento";
string tre = "Terzo elemento";
string quattro = "Quarto elemento";
string cinque = "Quinto elemento";

string vuoto = "";
string ch1 = "primo";
string ch2 = "secondo";
string ch3 = "terzo";
string ch4 = "quarto";
string ch5 = "quinto";

string chpippo = "pippo";
string empty = "";

string key13 = "key13";
string key15 = "key15";

bool free_called = false;

// Simple free function for testing clean method
void myfree(void *data)
{
    free_called = true;

    if(data != NULL)
    {
        free((dataload *)data);
    }

    return;
}

// dataobject free function
void dofree(void *data)
{
    free_called = true;

    dataobject *tofree = (dataobject *)data;

    if(tofree != NULL)
    {
        if(tofree->keyval != NULL)
        {
            free(tofree->keyval);
        }

        free(tofree);
    }

    return;
}

// worst hash function of the history
// just to check if it works, all inserts will result in collisions
unsigned int custom_hash(char *key, int dim)
{
    return dim/2;
}

TEST(FH, CustomHashFunction)
{
    fh_t *fh = NULL;

    // Create hash table of strings
    fh = fh_create(20, FH_DATALEN_STRING, custom_hash);
    ASSERT_NE((fh_t *)0, fh);

    int result = fh_insert(fh, (char *)"uno", (char *)"1");
    EXPECT_GE(result, 0);

    result = fh_insert(fh, (char *)"due", (char *)"2");
    EXPECT_GE(result, 0);

    EXPECT_EQ(1, fh->h_collision);

    EXPECT_EQ(FH_OK, fh_clean(fh, NULL));

    fh_destroy(fh);
}


// Simple test: creation, add, get, del and destroy a fast hashtable of simple strings. Check on hash table size after all the insert and after delete.
TEST(FH, create_add_get_del_destroy)
{
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add three elements
    int result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    // Check that hash table contains three elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(3, attribute);

    // Get one element and check returned value
    char *value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, due.c_str());

    // Delete an element
    result = fh_del(fhash, (char *)ch2.c_str());
    EXPECT_GT(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Check that hash table now contains two elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(2, attribute);

    // Get another element and check returned value
    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_OK, error);

    EXPECT_STREQ(value, tre.c_str());

    // Destroy hash table
    fh_destroy(fhash);
}

// Check all bad handle conditions
TEST(FH, bad_handle)
{
    char *value = NULL;
    int DIM = 100;
    char stringa[DIM];
    fh_t *fhash = NULL;
    int pos = 0, result = 0, error = 0, attribute = 0;
    char *key = NULL;
    void *slot = NULL;

    // Hash object not created: check all possible calls result

    // Destroy
    error = fh_destroy(fhash);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Get element
    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_BAD_HANDLE, error);
    EXPECT_STREQ(NULL, value);

    // Insert element
    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Delete element
    result = 0;
    result = fh_del(fhash, (char *)ch1.c_str());
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Sett attr
    result = 0;
    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 0);
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Get attr
    result = 0;
    result = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Search element
    result = 0;
    result = fh_search(fhash, (char *)ch3.c_str(), stringa, DIM);
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Scan start
    result = 0;
    result = fh_scan_start(fhash, 0, &slot);
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Scan next
    result = 0;
    result = fh_scan_next(fhash, &pos, &slot, key, stringa, DIM);
    EXPECT_EQ(result, FH_BAD_HANDLE);

    // Search lock
    error = 0;
    value = (char *)fh_searchlock(fhash, (char *)ch3.c_str(), &pos, &error);
    EXPECT_EQ(FH_BAD_HANDLE, error);
    EXPECT_STREQ(NULL, value);

    // Release lock
    error = fh_releaselock(fhash, pos);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Clean fh
    error = 0;
    error = fh_clean(fhash, NULL);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Create enumerator
    error = 0;
    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Get value from enumerator
    error = 0;
    fh_elem_t *element = fh_enum_get_value(fhe, &error);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Go to next element
    error = 0;
    error = fh_enum_move_next(fhe);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Destroy enumerator handle
    error = 0;
    error = fh_enum_destroy(fhe);
    EXPECT_EQ(FH_BAD_HANDLE, error);
}

// Check all error conditions (except bad handle) of methods that should return error values
TEST(FH, error_conditions)
{
    char *value = NULL;
    int DIM = 100;
    char stringa[DIM];
    fh_t *fhash = NULL;
    int pos = 0, result = 0, error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Get element out of an empty hash, check error returned
    value = (char *)fh_get(fhash, (char *)ch1.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Same of above, with search lock
    error = 0;
    value = (char *)fh_searchlock(fhash, (char *)ch3.c_str(), &pos, &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // All set attribute tests must be executed before first insert
    // Set attribute undefined
    result = 0;
    result = fh_setattr(fhash, 200, 0);
    EXPECT_EQ(result, FH_BAD_ATTR);

    // Set attribute with invalid value (actually value isn't used, method returns always 1 that is operation executed)
    result = 0;
    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 3000);
    EXPECT_EQ(result, 1);

    // Add an element with null key
    result = 0;
    result = fh_insert(fhash, NULL, (void *)uno.c_str());
    EXPECT_EQ(result, FH_INVALID_KEY);

    // Add an element with empty key
    result = 0;
    result = fh_insert(fhash, (char *)empty.c_str(), (void *)uno.c_str());
    EXPECT_EQ(result, 0);

    // Add an element with null value
    result = 0;
    result = fh_insert(fhash, (char *)chpippo.c_str(), NULL);
    EXPECT_GE(result, 0);

    // Add an element
    result = 0;
    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    // Add an element with the same key of the first one, check error returned
    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_EQ(result, FH_DUPLICATED_ELEMENT);

    // Get element not inserted, check error returned
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Get element with key NULL
    value = (char *)fh_get(fhash, NULL, &error);
    EXPECT_EQ(FH_INVALID_KEY, error);
    EXPECT_STREQ(NULL, value);

    // Same of above, with search lock
    error = 0;
    value = (char *)fh_searchlock(fhash, NULL, &pos, &error);
    EXPECT_EQ(FH_INVALID_KEY, error);
    EXPECT_STREQ(NULL, value);

    // Delete element with key NULL
    result = 0;
    result = fh_del(fhash, NULL);
    EXPECT_EQ(FH_INVALID_KEY, error);

    // Get attribute undefined
    result = 0;
    result = fh_getattr(fhash, 500, &attribute);
    EXPECT_EQ(result, FH_BAD_ATTR);

    // Search element with key NULL
    result = 0;
    result = fh_search(fhash, NULL, stringa, DIM);
    EXPECT_EQ(result, FH_INVALID_KEY);

    // Search element with buffer NULL
    result = 0;
    result = fh_search(fhash, (char *)ch1.c_str(), NULL, DIM);
    EXPECT_EQ(result, FH_BUFFER_NULL);

    // Search element with negative dimension
    result = 0;
    result = fh_search(fhash, (char *)ch1.c_str(), stringa, -2);
    EXPECT_EQ(result, FH_DIM_INVALID);

    // Clean fh
    error = 0;
    error = fh_clean(fhash, myfree);
    EXPECT_EQ(FH_FREE_NOT_REQUESTED, error);

    // Destroy hash table
    fh_destroy(fhash);
}

/// Create hashtable, put something in it, then destroy it. Try to access with all the methods and get BAD_HANDLE from everyone
TEST(FH, check_after_destroy)
{
    fh_t *fhash = NULL;
    int pos = 0, result = 0, error = 0, attribute = 0;
    int DIM = 100;
    char stringa[DIM];
    char *key = NULL;
    void *slot = NULL;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add three elements
    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    // Check that hash table contains three elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(3, attribute);

    // Get one element and check returned value
    char *value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_STREQ(value, due.c_str());

    // Destroy hash table
    fh_destroy(fhash);
}

TEST(FH, test_attr_methods)
{
    int DIM = 100;
    fh_t *fhash = NULL;
    int result = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(3, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Set attr: only one attribute should be set, result value 1 means success
    // This operation must be done before insert any data
    result = 0;
    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 1);
    EXPECT_EQ(result, 1);

    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch4.c_str(), (void *)quattro.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch5.c_str(), (void *)cinque.c_str());
    EXPECT_GE(result, 0);

    // Get attr: elements in hash
    result = 0;
    result = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(attribute, 5);

    // Get attr: real dimension of hash table (power of 2 value just greater than dimension specified in create)
    result = 0;
    attribute = 0;
    result = fh_getattr(fhash, FH_ATTR_DIM, &attribute);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(attribute, 4);

    // Get attr: number of collisiona
    result = 0;
    attribute = 0;
    result = fh_getattr(fhash, FH_ATTR_COLLISION, &attribute);
    EXPECT_EQ(result, 1);
    EXPECT_EQ(attribute, 2);

    // Destroy hash table
    fh_destroy(fhash);
}

TEST(FH, hash_with_struct)
{
    dataload *buffer;
    dataload value, newvalue;
    fh_t *fhash = NULL;
    int hashsize = 50;
    int result = 0;
    char key[10];
    int error;

    // Create hash table of structures
    fhash = fh_create(hashsize, sizeof(dataload), NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill it with data
    for(int index = 0; index < hashsize; index++)
    {
        buffer = (dataload *)malloc(sizeof(dataload));
        buffer->numval = index;
        strcpy(buffer->textval, "Text");
        strcat(buffer->textval, ToString(index).c_str());
        strcpy(key, "key");
        strcat(key, ToString(index).c_str());

        result = fh_insert(fhash, key, buffer);
        EXPECT_GE(result, 0);

        free(buffer);
    }

    // Get a copy of an element. Last parameter is datalen, but if the opaque object is a struct it is useless
    // (methods will use datalen set at creation time).
    result = fh_search(fhash, (char *)key15.c_str(), &value, sizeof(dataload));
    EXPECT_GE(result, 0);
    cout << "Value read (numval - textval): " << value.numval << " - " << value.textval << endl;

    // Check that modifying values in this buffer don't modify hashtable content
    value.numval = 200;
    strcpy(value.textval, "Modified");

    result = fh_search(fhash, (char *)key15.c_str(), &newvalue, sizeof(dataload));
    EXPECT_GE(result, 0);

    EXPECT_NE(value.numval, newvalue.numval);
    EXPECT_STRNE(value.textval, newvalue.textval);

    cout << "Value modified: " << value.numval << " - " << value.textval << endl;
    cout << "Value in hashtable: " << newvalue.numval << " - " << newvalue.textval << endl;

    // Search lock and modify element values
    int pos = 0;
    dataload *pvalue = (dataload *)fh_searchlock(fhash, (char *)key13.c_str(), &pos, &error);

    pvalue->numval = 80;
    strcpy(pvalue->textval, "Text80");

    // Release lock
    error = fh_releaselock(fhash, pos);
    EXPECT_EQ(FH_OK, error);

    // Get modified element
    dataload *valuemod = (dataload *)fh_get(fhash, (char *)key13.c_str(), &error);

    EXPECT_EQ(pvalue->numval, valuemod->numval);
    EXPECT_STREQ(pvalue->textval, valuemod->textval);

    cout << "Value modified: " << pvalue->numval << " - " << pvalue->textval << endl;
    cout << "Value in hashtable (modified too): " << valuemod->numval << " - " << valuemod->textval << endl;

    // Destroy hash table
    fh_destroy(fhash);
}

// Hash with elements null. Everything must be ok
TEST(FH, hashtable_with_null_elements)
{
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add three elements
    int result = fh_insert(fhash, (char *)ch1.c_str(), NULL);
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), NULL);
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), NULL);
    EXPECT_GE(result, 0);

    // Check that hash table contains three elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(3, attribute);

    // Get one element and check returned value
    char *value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_STREQ(value, NULL);

    // Delete an element
    result = fh_del(fhash, (char *)ch2.c_str());
    EXPECT_GT(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Check that hash table now contains two elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(2, attribute);

    // Get another element and check returned value
    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_STREQ(value, NULL);

    // Destroy hash table
    fh_destroy(fhash);
}

TEST(FH, test_setattr_dontcopykey)
{
    int DIM = 100;
    fh_t *fhash = NULL;
    int result = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(3, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Set attr: only one attribute should be set, result value 1 means success
    // This operation must be done before insert any data to be ok
    result = 0;
    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 1);
    EXPECT_EQ(result, 1);

    // Destroy hash table
    fh_destroy(fhash);

    // Create again hash table of strings. This time fill table, then try to set attribute FH_SETATTR_DONTCOPYKEY
    fhash = fh_create(3, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch4.c_str(), (void *)quattro.c_str());
    EXPECT_GE(result, 0);

    result = 0;
    result = fh_insert(fhash, (char *)ch5.c_str(), (void *)cinque.c_str());
    EXPECT_GE(result, 0);

    // This operation must fail
    result = 0;
    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 1);
    EXPECT_EQ(result, FH_ERROR_OPERATION_NOT_PERMITTED);

    // Destroy hash table
    fh_destroy(fhash);
}

TEST(FH, get_enumerator_and_list)
{
    fh_t *fhash = NULL;
    int error = 0;
    int hashsize = 20;
    char key[10];

    // Create hash table of strings
    fhash = fh_create(hashsize, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill it with data
    for(int index = 0; index < hashsize; index++)
    {
        char *buffer = (char *)malloc(15);
        strcpy(buffer, "Text");
        strcat(buffer, ToString(index).c_str());
        strcpy(key, "key");
        strcat(key, ToString(index).c_str());

        int result = fh_insert(fhash, key, buffer);
        EXPECT_GE(result, 0);

        free(buffer);
    }

    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    ASSERT_NE((fh_enum_t *)0, fhe);

    fh_elem_t *previous = NULL;
    int counter = 0;

    cout << "Read elements in alphabetical order" << endl;
    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &error);
        EXPECT_NE((fh_elem_t *)0, element);
        //cout << "Element read: " << element->key << endl;

        if(previous != NULL)
        {
            // Check if sort order is ok
            EXPECT_GT(strcmp(element->key, previous->key), 0);
        }

        if(element != NULL)
        {
            previous = element;
            counter++;
        }

        error = fh_enum_move_next(fhe);
        if(error == FH_BAD_HANDLE)
        {
            break;
        }
    }

    // Check if all elements in enumerator were read
    EXPECT_EQ(counter, fhe->size);

    fh_enum_destroy(fhe);

// ATTENTION!!!! Check of invalid enumerator don't work on MinGW (it seems that magic number won't be set to zero).
// On MacOs and Linux everything's ok.
    // Trying access to enumerator after destroy: must get a BAD_HANDLE
//    error = 0;
//    fh_elem_t *element = fh_enum_get_value(fhe, &error);
//    EXPECT_EQ(error, FH_BAD_HANDLE);
//
//    error = 0;
//    error = fh_enum_move_next(fhe);
//    EXPECT_EQ(error, FH_BAD_HANDLE);

    // Same test, but reverse order
    fhe = fh_enum_create(fhash, FH_ENUM_SORTED_DESC, &error);
    ASSERT_NE((fh_enum_t *)0, fhe);

    previous = NULL;
    counter = 0;

    cout << "Read elements in reverse alphabetical order" << endl;
    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &error);
        EXPECT_NE((fh_elem_t *)0, element);
        //cout << "Element read: " << element->key << endl;

        if(previous != NULL)
        {
            // Check if sort order is ok
            EXPECT_LT(strcmp(element->key, previous->key), 0);
        }

        if(element != NULL)
        {
            previous = element;
            counter++;
        }

        error = fh_enum_move_next(fhe);
        if(error == FH_BAD_HANDLE)
        {
            break;
        }
    }

    // Check if all elements in enumerator were read
    EXPECT_EQ(counter, fhe->size);

    fh_enum_destroy(fhe);

    // Destroy hash table
    fh_destroy(fhash);
}

TEST(FH, get_enumerator_with_empty_list)
{
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    EXPECT_EQ((fh_enum_t *)0, fhe);
    EXPECT_EQ(FH_EMPTY_HASHTABLE, error);

    error = fh_enum_destroy(fhe);
    EXPECT_EQ(FH_BAD_HANDLE, error);

    // Destroy hash table
    fh_destroy(fhash);
}

// Test clean method on string elements
TEST(FH, hf_string_clean_test)
{
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add three elements
    int result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    // Check that hash table contains three elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(3, attribute);

    // Get one element and check returned value
    char *value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_STREQ(value, due.c_str());

    // Clean hashtable
    free_called = false;
    error = fh_clean(fhash, NULL);
    EXPECT_EQ(FH_OK, error);
    EXPECT_EQ(false, free_called);

    // Check that hash table is empty
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(0, attribute);

    // Get one element and check returned value
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);

    // Destroy hash table
    fh_destroy(fhash);
}

// Test clean method on struct elements
TEST(FH, hf_struct_clean_test)
{
    dataload *buffer;
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;
    int hashsize = 50;
    int result = 0;
    char key[10];

    // Create hash table of structures
    fhash = fh_create(hashsize, sizeof(dataload), NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill it with data
    for(int index = 0; index < hashsize; index++)
    {
        buffer = (dataload *)malloc(sizeof(dataload));
        buffer->numval = index;
        strcpy(buffer->textval, "Text");
        strcat(buffer->textval, ToString(index).c_str());
        strcpy(key, "key");
        strcat(key, ToString(index).c_str());

        result = fh_insert(fhash, key, buffer);
        EXPECT_GE(result, 0);

        free(buffer);
    }

    // Check that hash table contains the right number of elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(hashsize, attribute);

    // Get one element and check returned value
    dataload *value = (dataload *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_NE((dataload *)0, value);

    // Clean hashtable
    free_called = false;
    error = fh_clean(fhash, NULL);
    EXPECT_EQ(FH_OK, error);
    EXPECT_EQ(false, free_called);

    // Check that hash table is empty
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(0, attribute);

    // Get one element and check returned value
    value = (dataload *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);

    // Destroy hash table
    fh_destroy(fhash);
}

// Test clean method on void pointer elements
TEST(FH, hf_void_clean_test)
{
    dataload *buffer;
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;
    int hashsize = 50;
    int result = 0;
    char key[10];

    // Create hash table of void pointers
    fhash = fh_create(hashsize, FH_DATALEN_VOIDP, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill it with data
    for(int index = 0; index < hashsize; index++)
    {
        buffer = (dataload *)malloc(sizeof(dataload));
        buffer->numval = index;
        strcpy(buffer->textval, "Text");
        strcat(buffer->textval, ToString(index).c_str());
        strcpy(key, "key");
        strcat(key, ToString(index).c_str());

        result = fh_insert(fhash, key, buffer);
        EXPECT_GE(result, 0);
    }

    // Check that hash table contains the right number of elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(hashsize, attribute);

    // Get one element and check returned value
    dataload *value = (dataload *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_NE((dataload *)0, value);

    // Clean hashtable
    free_called = false;
    error = fh_clean(fhash, myfree);
    EXPECT_EQ(FH_OK, error);
    EXPECT_EQ(true, free_called);

    // Check that hash table is empty
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(0, attribute);

    // Get one element and check returned value
    value = (dataload *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);

    // Destroy hash table
    fh_destroy(fhash);
}

// Test clean method on void pointer elements containing key values (don't copy key attribute set)
TEST(FH, hf_void_containing_key_clean_test)
{
    dataobject *buffer;
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;
    int hashsize = 50;
    int result = 0;

    // Create hash table of void pointers
    fhash = fh_create(hashsize, FH_DATALEN_VOIDP, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    result = fh_setattr(fhash, FH_SETATTR_DONTCOPYKEY, 1);
    EXPECT_EQ(result, 1);

    // Fill it with data
    for(int index = 0; index < hashsize; index++)
    {
        buffer = (dataobject *)malloc(sizeof(dataobject));
        buffer->keyval = (char *)malloc(10);
        buffer->numval = index;
        strcpy(buffer->textval, "Text");
        strcat(buffer->textval, ToString(index).c_str());
        strcpy(buffer->keyval, "key");
        strcat(buffer->keyval, ToString(index).c_str());

        result = fh_insert(fhash, buffer->keyval, buffer);
        EXPECT_GE(result, 0);
    }

    // Check that hash table contains the right number of elements
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(hashsize, attribute);

    // Get one element and check returned value
    dataobject *value = (dataobject *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_NE((dataobject *)0, value);

    // Clean hashtable
    free_called = false;
    error = fh_clean(fhash, dofree);
    EXPECT_EQ(FH_OK, error);
    EXPECT_EQ(true, free_called);

    // Check that hash table is empty
    error = fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);
    EXPECT_EQ(0, attribute);

    // Get one element and check returned value
    value = (dataobject *)fh_get(fhash, (char *)key15.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);

    // Destroy hash table
    fh_destroy(fhash);
}

// Test of hashing: hashtable with length 1, add four elements, delete elements one by one (order: last, first, second, third) checking that
// elements still not deleted are valid
TEST(FH, hashing_test)
{
    fh_t *fhash = NULL;
    int error = 0, attribute = 0;

    // Create hash table of strings
    fhash = fh_create(1, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add four elements
    int result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch4.c_str(), (void *)quattro.c_str());
    EXPECT_GE(result, 0);

    // Get last element and check returned value
    char *value = (char *)fh_get(fhash, (char *)ch4.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, quattro.c_str());

    // Delete last element
    result = fh_del(fhash, (char *)ch4.c_str());
    EXPECT_GE(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch4.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Get other elements and check their values
    value = (char *)fh_get(fhash, (char *)ch1.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, uno.c_str());

    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, due.c_str());

    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, tre.c_str());

    // Delete first element
    result = fh_del(fhash, (char *)ch1.c_str());
    EXPECT_GE(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch1.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Get other elements and check their values
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, due.c_str());

    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, tre.c_str());

    // Delete second element
    result = fh_del(fhash, (char *)ch2.c_str());
    EXPECT_GE(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Get last element remained and check its values
    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_OK, error);
    EXPECT_STREQ(value, tre.c_str());

    // Delete last element
    result = fh_del(fhash, (char *)ch3.c_str());
    EXPECT_GE(result, 0);

    // Get the element just deleted and check that nothing is returned (check error FH_ELEMENT_NOT_FOUND returned by get method)
    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, error);
    EXPECT_STREQ(NULL, value);

    // Destroy hash table
    fh_destroy(fhash);
}

// Create a table with an empty key, to see if there are problems
TEST(FH, hash_with_empty_key)
{
    fh_t *fhash = NULL;
    int pos = 0, result = 0, error = 0, attribute = 0;
    int DIM = 100;
    char stringa[DIM];
    char *key = NULL;
    void *slot = NULL;

    // Create hash table of strings
    fhash = fh_create(20, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Add some elementsm last one with empty key
    result = fh_insert(fhash, (char *)ch1.c_str(), (void *)uno.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch2.c_str(), (void *)due.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch3.c_str(), (void *)tre.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch4.c_str(), (void *)quattro.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)ch5.c_str(), (void *)cinque.c_str());
    EXPECT_GE(result, 0);

    result = fh_insert(fhash, (char *)vuoto.c_str(), (void *)vuoto.c_str());
    EXPECT_GE(result, 0);

    // Get every element and check returned value
    char *value = (char *)fh_get(fhash, (char *)vuoto.c_str(), &error);
    EXPECT_STREQ(value, (char *)vuoto.c_str());

    value = (char *)fh_get(fhash, (char *)ch1.c_str(), &error);
    EXPECT_STREQ(value, (char *)uno.c_str());

    value = (char *)fh_get(fhash, (char *)ch2.c_str(), &error);
    EXPECT_STREQ(value, (char *)due.c_str());

    value = (char *)fh_get(fhash, (char *)ch3.c_str(), &error);
    EXPECT_STREQ(value, (char *)tre.c_str());

    value = (char *)fh_get(fhash, (char *)ch4.c_str(), &error);
    EXPECT_STREQ(value, (char *)quattro.c_str());

    value = (char *)fh_get(fhash, (char *)ch5.c_str(), &error);
    EXPECT_STREQ(value, (char *)cinque.c_str());

    // Delete element with empty key
    result = fh_del(fhash, (char *)vuoto.c_str());
    EXPECT_GE(result, 0);

    // Check that element with empty key was removed
    value = (char *)fh_get(fhash, (char *)vuoto.c_str(), &error);
    EXPECT_STREQ(value, NULL);

    // Destroy hash table
    fh_destroy(fhash);
}

// this was a fh_del() bug causing a memory leak and breaking the whole universe
// see also INFUZE-833
TEST(FH, DeleteUnexistantKeyAndInsert)
{
    fh_t *fh = fh_create(16, FH_DATALEN_STRING, NULL);
    EXPECT_NE(nullptr, fh);

    const char *header = "key";
    const char *value = "value";

    EXPECT_EQ(FH_ELEMENT_NOT_FOUND, fh_del(fh, (char *) header));
    EXPECT_LE(0, fh_insert(fh, (char *) header, (void *) value));

    EXPECT_EQ(FH_OK, fh_destroy(fh));
}
