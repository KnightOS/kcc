/** Tests many of the basic operators from each of the storage types to every other.

    source_storage: static, register,
    dest_storage: static, register, 
    type: char, int
 */
#include <testfwk.h>

/** Simple function that spoils sdcc's optimiser by hiding an assign.
 */
static {type}
spoilAssign({type} in)
{
    return in;
}

static void
testStorageTypes(void)
{
    {source_storage} {type} source;
    {dest_storage} {type} dest;

    source = spoilAssign(17);
    // Test compare against a const
    ASSERT(source == 17);

    dest = spoilAssign(134);
    ASSERT(dest == 134);
    ASSERT(dest != source);

    // Test assignment
    dest = source;
    ASSERT(dest == source);

    // Test cmp
    dest--;
    ASSERT(dest == 16);
    ASSERT(dest < source);
    
    dest += 8;
    ASSERT(dest == 24);
    ASSERT(source < dest);
}
