#include <CUnit/Basic.h>

int init_config(void)
{
  return 0;
}

int clean_config(void)
{
  return 0;
}

void test_config1(void)
{
  CU_ASSERT_EQUAL(1, 1);
  CU_ASSERT_EQUAL(1, 4);
}

int main()
{
  CU_pSuite pSuite = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();
 
  /* add a suite to the registry */
  pSuite = CU_add_suite("Lua: config", init_config, clean_config);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* add the tests to the suite */
  /* NOTE - ORDER IS IMPORTANT - MUST TEST fread() AFTER fprintf() */
  if ((NULL == CU_add_test(pSuite, "test of config1()", test_config1))// ||
      //      (NULL == CU_add_test(pSuite, "test of fread()", testFREAD))
      )
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
