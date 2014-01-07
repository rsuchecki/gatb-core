/*****************************************************************************
 *   GATB : Genome Assembly Tool Box                                         *
 *   Copyright (c) 2013                                                      *
 *                                                                           *
 *   GATB is free software; you can redistribute it and/or modify it under   *
 *   the CECILL version 2 License, that is compatible with the GNU General   *
 *   Public License                                                          *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   CECILL version 2 License for more details.                              *
 *****************************************************************************/

#include <CppunitCommon.hpp>

#include <gatb/system/impl/System.hpp>

#include <gatb/tools/collections/impl/Product.hpp>
#include <gatb/tools/collections/impl/ProductFile.hpp>
#include <gatb/tools/collections/impl/ProductHDF5.hpp>
#include <gatb/tools/collections/impl/CollectionCache.hpp>

#include <gatb/tools/misc/api/Range.hpp>

#include <gatb/tools/designpattern/impl/Command.hpp>

#include <gatb/tools/misc/api/Macros.hpp>
#include <gatb/tools/math/NativeInt64.hpp>
#include <gatb/tools/math/LargeInt.hpp>

using namespace std;

using namespace gatb::core::tools::collections;
using namespace gatb::core::tools::collections::impl;

using namespace gatb::core::tools::dp;
using namespace gatb::core::tools::dp::impl;

using namespace gatb::core::tools::misc;

using namespace gatb::core::tools::math;

using namespace gatb::core::system;
using namespace gatb::core::system::impl;

#define ABS(a)  ((a)<0 ? -(a) : (a))

/********************************************************************************/
namespace gatb  {  namespace tests  {
/********************************************************************************/

/** \brief Test class for operating system operations
 */
class TestCollection : public Test
{
    /********************************************************************************/
    CPPUNIT_TEST_SUITE_GATB (TestCollection);

        CPPUNIT_TEST_GATB (collection_check1);
        CPPUNIT_TEST_GATB (collection_check2);
        CPPUNIT_TEST_GATB (collection_check3);
        CPPUNIT_TEST_GATB (collection_check4);
        CPPUNIT_TEST_GATB (collection_check5);

        CPPUNIT_TEST_GATB (collection_HDF5_check_collection);
        CPPUNIT_TEST_GATB (collection_HDF5_check_partition);

    CPPUNIT_TEST_SUITE_GATB_END();

public:

    /********************************************************************************/
    void setUp    ()  {}
    void tearDown ()  {}

    /********************************************************************************/
    struct Test
    {
        char a;  int n;  float x;
        Test(char a=0, int n=0, float x=0) : a(a), n(n), x(x) {}
        bool operator== (const Test& t)  { return a==t.a && n==t.n && x==t.x; }
    };

    /********************************************************************************/
    template<typename T> void collection_check1_aux (T* table, size_t nb)
    {
        /** We create a collection. */
        Collection<T>* c = new CollectionFile<T> ("foo");
        LOCAL(c);

        /** We insert some items. */
        for (size_t i=0; i<nb; i++)  { c->insert (table[i]); }

        /** We have to flush to be sure all items are in the collection. */
        c->flush();

        /** We check we have the correct number of items. */
        CPPUNIT_ASSERT (c->getNbItems() == nb);

        /** We iterate the collection. */
        Iterator<T>* it = c->iterator();
        LOCAL(it);

        /** We check we get the correct items. */
        size_t i=0;
        for (it->first(); !it->isDone(); it->next())  {  CPPUNIT_ASSERT (it->item() == table[i++]);  }

        /** We delete physically the collection. */
        //c->remove ();
    }

    /********************************************************************************/
    void collection_check1 ()
    {
        int table1[] = { 1,2,3,5,8,13,21,34};
        collection_check1_aux<int> (table1, ARRAY_SIZE(table1));

        string table2[] = { "a", "bc", "def", "ghij"};
        collection_check1_aux<string> (table2, ARRAY_SIZE(table2));

        // Test table3[] = { Test(1, 1025, 3.14), Test(3, 298, 2.713), Test(19, 9874, 0.577)};
        // collection_check1_aux<Test> (table3, ARRAY_SIZE(table3));

        NativeInt64 table4[] = { 12354684684, 6876436549, 87654351, 6843516877, 68435434874};
        collection_check1_aux<NativeInt64> (table4, ARRAY_SIZE(table4));

        LargeInt<3> table5[] = { LargeInt<3>(413434), LargeInt<3>(987654123), LargeInt<3>(123), LargeInt<3>(1) };
        collection_check1_aux<LargeInt<3> > (table5, ARRAY_SIZE(table5));
    }

    /********************************************************************************/
    void collection_check2 ()
    {
        /** We create a product. */
        Product<ProductFileFactory> product ("dsk");

        /** We get a collection from the product. */
        Collection<NativeInt64>& solidKmers = product().getCollection<NativeInt64> ("solid");

        NativeInt64 table[] = { 12354684684, 6876436549, 87654351, 6843516877, 68435434874};
        vector<NativeInt64> items (table, table + ARRAY_SIZE(table));

        /** We insert some items. */
        solidKmers.insert (items);

        /** We have to flush the bag in order to be sure all items are correctly inserted. */
        solidKmers.flush ();

        /** We check we inserted the correct number of items. */
        CPPUNIT_ASSERT (solidKmers.getNbItems() == ARRAY_SIZE(table));

        /** We delete the product. */
        product.remove ();
    }

    /********************************************************************************/
    void collection_check3 ()
    {
        size_t nbParts = 10;

        /** We create a product. */
        Product<ProductFileFactory> product ("graph");

        /** We create a partition. */
        Partition<ProductFileFactory,NativeInt64>& partition = product().getPartition<NativeInt64> ("parts", nbParts);

        CPPUNIT_ASSERT (partition.size() == nbParts);

        /** We insert some items into the partition. */
        for (size_t i=0; i<partition.size(); i++)  {  partition[i].insert (2*i);  }

        /** We flush the partition. */
        partition.flush ();

        /** We check the content of what was inserted. */
        for (size_t i=0; i<partition.size(); i++)
        {
            size_t nbIter = 0;
            Iterator<NativeInt64>* it = partition[i].iterator();    LOCAL(it);
            for (it->first(); !it->isDone(); it->next(), nbIter++)  {  CPPUNIT_ASSERT (it->item() == 2*i);  }
            CPPUNIT_ASSERT (nbIter == 1);
        }

        /** We use a partition cache. */
        PartitionCache<ProductFileFactory,NativeInt64> cache (partition, 10000);

        /** We insert some items into the partition. */
        for (size_t i=0; i<cache.size(); i++)  {  cache[i].insert (2*i+1);  }

        /** We flush the cache. */
        cache.flush ();

        /** We check the content of what was inserted. */
        for (size_t i=0; i<partition.size(); i++)
        {
            size_t nbIter = 0;
            Iterator<NativeInt64>* it = partition[i].iterator();    LOCAL(it);
            for (it->first(); !it->isDone(); it->next(), nbIter++)
            {
                if (nbIter==0)  {  CPPUNIT_ASSERT (it->item() == 2*i  ); }
                if (nbIter==1)  {  CPPUNIT_ASSERT (it->item() == 2*i+1); }
            }
            CPPUNIT_ASSERT (nbIter == 2);
        }

        /** We delete the product. */
        product.remove ();
    }

    /********************************************************************************/
    void collection_check4 ()
    {
        size_t nbParts = 10;
        size_t nbItems = nbParts * 1000;

        /** We create a product. */
        Product<ProductFileFactory> product ("graph");

        /** We create a partition. */
        Partition<ProductFileFactory,NativeInt64>& partition = product().getPartition<NativeInt64> ("parts", nbParts);

        /** We add items in partitions. */
        for (size_t i=0; i<nbItems; i++)  {  partition[i%nbParts].insert (i);  }

        /** We flush the partition. */
        partition.flush ();

        /** We check the content of each partition. */
        for (size_t i=0; i<nbParts; i++)
        {
            size_t nbIter = 0;
            Iterator<NativeInt64>* it = partition[i].iterator();    LOCAL(it);
            for (it->first(); !it->isDone(); it->next(), nbIter++)
            {
                // entry should be for some k : nbParts.k + i
                CPPUNIT_ASSERT ( (it->item() - i) % nbParts == 0);
            }
            CPPUNIT_ASSERT (nbIter == nbItems / nbParts);
        }

        /** We delete the product. */
        product.remove ();
    }

    /********************************************************************************/

    class MyFunctor
    {
    public:

        MyFunctor (Partition<ProductFileFactory,NativeInt64>& partition, size_t nbRepeat)
            : _cache (partition, 10000, System::thread().newSynchronizer()), _nbParts(partition.size()), _nbRepeat(nbRepeat)   {}

        void operator() (const size_t& item)
        {
            /** We repeat the process => more job to do on each thread. */
            for (size_t i=0; i<_nbRepeat; i++)   {  _cache [item % _nbParts].insert (item);  }
        }

    private:

        PartitionCache<ProductFileFactory,NativeInt64> _cache;
        size_t _nbParts;
        size_t _nbRepeat;
    };

    void collection_check5 ()
    {
        size_t nbParts  = 10;
        size_t nbItems  = nbParts * 1000;
        size_t nbRepeat = 500;

        /** We create a product. */
        Product<ProductFileFactory> product ("graph");

        /** We create a partition. */
        Partition<ProductFileFactory,NativeInt64>& partition = product().getPartition<NativeInt64> ("parts", nbParts);

        /** We build a Range. */
        Range<size_t>::Iterator it (0, nbItems-1);

        /** We dispatch the range iteration. */
        Dispatcher().iterate (it, MyFunctor(partition,nbRepeat));

        /** We check the content of each partition. */
        size_t nbIterTotal = 0;
        for (size_t i=0; i<nbParts; i++)
        {
            size_t nbIter = 0;
            Iterator<NativeInt64>* it = partition[i].iterator();    LOCAL(it);
            for (it->first(); !it->isDone(); it->next(), nbIter++)
            {
                // each entry should be (for some k) : nbParts.k + i
                CPPUNIT_ASSERT ( (it->item() - i) % nbParts == 0);
            }
            CPPUNIT_ASSERT (nbIter == (nbRepeat * nbItems) / nbParts);

            nbIterTotal += nbIter;
        }

        /** We check the total number of items. */
        CPPUNIT_ASSERT (nbIterTotal == nbRepeat * nbItems);

        /** We delete the product. */
        product.remove ();
    }

    /********************************************************************************/
    template<typename T>
    void collection_HDF5_check_collection_aux (T* values, size_t len)
    {
        /** We create a product. */
        Product<ProductHDF5Factory>* product = ProductHDF5Factory::createProduct ("aProduct", true, false);
        LOCAL (product);

        /** We get a collection from the product. */
        Collection<T>& collection = (*product)().getCollection<T> ("foo");

        /** We insert the values into the bag. */
        collection.insert (values, len);

        CPPUNIT_ASSERT (collection.getNbItems() == len);

        /** We get an iterator on the collection. */
        size_t idx=0;
        Iterator<T>* it = collection.iterator();  LOCAL(it);
        for (it->first(); !it->isDone(); it->next(), idx++)
        {
            CPPUNIT_ASSERT (it->item() == values[idx]);
        }

        CPPUNIT_ASSERT (idx == len);

        /** We remove physically the product. */
        product->remove ();
    }

    /********************************************************************************/
    void collection_HDF5_check_collection ()
    {
        NativeInt64 values1[] = { 1,2,3,4,5,6,7,8,9};
        collection_HDF5_check_collection_aux (values1, ARRAY_SIZE(values1));

        NativeInt64 values2[4096];  for (size_t i=0; i<ARRAY_SIZE(values2); i++)  { values2[i]=i; }
        collection_HDF5_check_collection_aux (values2, ARRAY_SIZE(values2));
    }

    /********************************************************************************/
    template<typename T>
    void collection_HDF5_check_partition_aux (T* values, size_t len, size_t nbParts)
    {
        /** We create a product. */
        Product<ProductHDF5Factory>* product = ProductHDF5Factory::createProduct ("aProduct", true, false);
        LOCAL (product);

        /** We create a partition. */
        Partition<ProductHDF5Factory,T>& partition = (*product)().getPartition <T> ("foo", nbParts);

        /** We cache the partition. */
        PartitionCache<ProductHDF5Factory,T> cache (partition, 1<<10, 0);

        /** We insert the items. */
        for (size_t i=0; i<len; i++)  {  cache[i%nbParts].insert (values[i]);  }

        /** We flush the partition. */
        cache.flush ();

        /** We check the content of each partition. */
        for (size_t p=0; p<nbParts; p++)
        {
            /** We get an iterator on the current collection. */
            size_t idx=0;
            Iterator<T>* it = cache[p].iterator();  LOCAL(it);
            for (it->first(); !it->isDone(); it->next(), idx++)
            {
                CPPUNIT_ASSERT (it->item() == values[idx* nbParts + p]);
            }

            CPPUNIT_ASSERT (idx == len/4);
        }

        /** We remove physically the product. */
        product->remove ();
    }

    /********************************************************************************/
    void collection_HDF5_check_partition ()
    {
        NativeInt64 values1[1<<15];  for (size_t i=0; i<ARRAY_SIZE(values1); i++)  { values1[i]=i; }
        collection_HDF5_check_partition_aux (values1, ARRAY_SIZE(values1), 4);
    }
};

/********************************************************************************/

CPPUNIT_TEST_SUITE_REGISTRATION      (TestCollection);
CPPUNIT_TEST_SUITE_REGISTRATION_GATB (TestCollection);

/********************************************************************************/
} } /* end of namespaces. */
/********************************************************************************/
