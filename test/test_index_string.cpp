#include "tightdb/index_string.hpp"
#include <UnitTest++.h>

using namespace tightdb;

namespace {
    // strings used by tests
    const char s1[] = "John";
    const char s2[] = "Brian";
    const char s3[] = "Samantha";
    const char s4[] = "Tom";
    const char s5[] = "Johnathan";
    const char s6[] = "Johnny";
    const char s7[] = "Sam";

} // namespace

TEST(StringIndex_BuildIndex)
{
    // Create a column with string values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value
    col.add(s5); // common prefix
    col.add(s6); // common prefix

    // Create a new index on column
    const StringIndex& ndx = col.create_index();

    const size_t r1 = ndx.find_first(s1);
    const size_t r2 = ndx.find_first(s2);
    const size_t r3 = ndx.find_first(s3);
    const size_t r4 = ndx.find_first(s4);
    const size_t r5 = ndx.find_first(s5);
    const size_t r6 = ndx.find_first(s6);

    CHECK_EQUAL(0, r1);
    CHECK_EQUAL(1, r2);
    CHECK_EQUAL(2, r3);
    CHECK_EQUAL(3, r4);
    CHECK_EQUAL(5, r5);
    CHECK_EQUAL(6, r6);

    // Clean up
    col.destroy();
}

TEST(StringIndex_DeleteAll)
{
    // Create a column with string values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value
    col.add(s5); // common prefix
    col.add(s6); // common prefix

    // Create a new index on column
    const StringIndex& ndx = col.create_index();

    // Delete all entries
    // (reverse order to avoid ref updates)
    col.erase(6, 6 == col.size()-1);
    col.erase(5, 5 == col.size()-1);
    col.erase(4, 4 == col.size()-1);
    col.erase(3, 3 == col.size()-1);
    col.erase(2, 2 == col.size()-1);
    col.erase(1, 1 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
#ifdef TIGHTDB_DEBUG
    CHECK(ndx.is_empty());
#else
    static_cast<void>(ndx);
#endif

    // Re-insert values
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value
    col.add(s5); // common prefix
    col.add(s6); // common prefix

    // Delete all entries
    // (in order to force constant ref updating)
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
#ifdef TIGHTDB_DEBUG
    CHECK(ndx.is_empty());
#else
    static_cast<void>(ndx);
#endif

    // Clean up
    col.destroy();
}

TEST(StringIndex_Delete)
{
    // Create a column with random values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value

    // Create a new index on column
    const StringIndex& ndx = col.create_index();

    // Delete first item (in index)
    col.erase(1, 1 == col.size()-1);

    CHECK_EQUAL(0, col.find_first(s1));
    CHECK_EQUAL(1, col.find_first(s3));
    CHECK_EQUAL(2, col.find_first(s4));
    CHECK_EQUAL(not_found, ndx.find_first(s2));

    // Delete last item (in index)
    col.erase(2, 2 == col.size()-1);

    CHECK_EQUAL(0, col.find_first(s1));
    CHECK_EQUAL(1, col.find_first(s3));
    CHECK_EQUAL(not_found, col.find_first(s4));
    CHECK_EQUAL(not_found, col.find_first(s2));

    // Delete middle item (in index)
    col.erase(1, 1 == col.size()-1);

    CHECK_EQUAL(0, col.find_first(s1));
    CHECK_EQUAL(not_found, col.find_first(s3));
    CHECK_EQUAL(not_found, col.find_first(s4));
    CHECK_EQUAL(not_found, col.find_first(s2));

    // Delete all items
    col.erase(0, 0 == col.size()-1);
    col.erase(0, 0 == col.size()-1);
#ifdef TIGHTDB_DEBUG
    CHECK(ndx.is_empty());
#endif

    // Clean up
    col.destroy();
}

TEST(StringIndex_Insert)
{
    // Create a column with random values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value

    // Create a new index on column
    col.create_index();

    // Insert item in top of column
    col.insert(0, s5);

    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s1));
    CHECK_EQUAL(2, col.find_first(s2));
    CHECK_EQUAL(3, col.find_first(s3));
    CHECK_EQUAL(4, col.find_first(s4));
    //CHECK_EQUAL(5, ndx.find_first(s1)); // duplicate

    // Append item in end of column
    col.insert(6, s6);

    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s1));
    CHECK_EQUAL(2, col.find_first(s2));
    CHECK_EQUAL(3, col.find_first(s3));
    CHECK_EQUAL(4, col.find_first(s4));
    CHECK_EQUAL(6, col.find_first(s6));

    // Insert item in middle
    col.insert(3, s7);

    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s1));
    CHECK_EQUAL(2, col.find_first(s2));
    CHECK_EQUAL(3, col.find_first(s7));
    CHECK_EQUAL(4, col.find_first(s3));
    CHECK_EQUAL(5, col.find_first(s4));
    CHECK_EQUAL(7, col.find_first(s6));

    // Clean up
    col.destroy();
}

TEST(StringIndex_Set)
{
    // Create a column with random values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s3);
    col.add(s4);
    col.add(s1); // duplicate value

    // Create a new index on column
    col.create_index();

    // Set top value
    col.set(0, s5);

    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s2));
    CHECK_EQUAL(2, col.find_first(s3));
    CHECK_EQUAL(3, col.find_first(s4));
    CHECK_EQUAL(4, col.find_first(s1));

    // Set bottom value
    col.set(4, s6);

    CHECK_EQUAL(not_found, col.find_first(s1));
    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s2));
    CHECK_EQUAL(2, col.find_first(s3));
    CHECK_EQUAL(3, col.find_first(s4));
    CHECK_EQUAL(4, col.find_first(s6));

    // Set middle value
    col.set(2, s7);

    CHECK_EQUAL(not_found, col.find_first(s3));
    CHECK_EQUAL(not_found, col.find_first(s1));
    CHECK_EQUAL(0, col.find_first(s5));
    CHECK_EQUAL(1, col.find_first(s2));
    CHECK_EQUAL(2, col.find_first(s7));
    CHECK_EQUAL(3, col.find_first(s4));
    CHECK_EQUAL(4, col.find_first(s6));

    // Clean up
    col.destroy();
}

TEST(StringIndex_Count)
{
    // Create a column with duplcate values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s2);
    col.add(s3);
    col.add(s3);
    col.add(s3);
    col.add(s4);
    col.add(s4);
    col.add(s4);
    col.add(s4);

    // Create a new index on column
    col.create_index();

    // Counts
    const size_t c0 = col.count(s5);
    const size_t c1 = col.count(s1);
    const size_t c2 = col.count(s2);
    const size_t c3 = col.count(s3);
    const size_t c4 = col.count(s4);
    CHECK_EQUAL(0, c0);
    CHECK_EQUAL(1, c1);
    CHECK_EQUAL(2, c2);
    CHECK_EQUAL(3, c3);
    CHECK_EQUAL(4, c4);

    // Clean up
    col.destroy();
}

TEST(StringIndex_Distinct)
{
    // Create a column with duplcate values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s2);
    col.add(s3);
    col.add(s3);
    col.add(s3);
    col.add(s4);
    col.add(s4);
    col.add(s4);
    col.add(s4);

    // Create a new index on column
    StringIndex& ndx = col.create_index();

    // Get view of unique values
    // (sorted in alphabetical order, each ref to first match)
    Array result;
    ndx.distinct(result);

    CHECK_EQUAL(4, result.size());
    CHECK_EQUAL(1, result[0]); // s2 = Brian
    CHECK_EQUAL(0, result[1]); // s1 = John
    CHECK_EQUAL(3, result[2]); // s3 = Samantha
    CHECK_EQUAL(6, result[3]); // s4 = Tom

    // Clean up
    result.destroy();
    col.destroy();
}

#if 0 // fixme
TEST(StringIndex_FindAllNoCopy)
{
    // Create a column with duplcate values
    AdaptiveStringColumn col;
    col.add(s1);
    col.add(s2);
    col.add(s2);
    col.add(s3);
    col.add(s3);
    col.add(s3);
    col.add(s4);
    col.add(s4);
    col.add(s4);
    col.add(s4);

    // Create a new index on column
    StringIndex& ndx = col.CreateIndex();

    size_t ref = not_found;
    FindRes res1 = ndx.find_all("not there", ref);
    CHECK_EQUAL(FindRes_not_found, res1);

    FindRes res2 = ndx.find_all(s1, ref);
    CHECK_EQUAL(FindRes_single, res2);
    CHECK_EQUAL(0, ref);

    FindRes res3 = ndx.find_all(s4, ref);
    CHECK_EQUAL(FindRes_column, res3);
    const Column results(ref);
    CHECK_EQUAL(4, results.Size());
    CHECK_EQUAL(6, results.Get(0));
    CHECK_EQUAL(7, results.Get(1));
    CHECK_EQUAL(8, results.Get(2));
    CHECK_EQUAL(9, results.Get(3));

    // Clean up
    col.destroy();
}
#endif
