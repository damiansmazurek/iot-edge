// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "testrunnerswitcher.h"
#include "umock_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

static void* my_gballoc_malloc(size_t size)
{
	return malloc(size);
}

static void my_gballoc_free(void* s)
{
	free(s);
}


#define ENABLE_MOCKS
#include "module.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/httpapiex.h"
#include "message.h"
#include "azure_c_shared_utility/base64.h"
#include "azure_c_shared_utility/gballoc.h"

MOCKABLE_FUNCTION(, const CONSTBUFFER*, Message_GetContent, MESSAGE_HANDLE, message);

#undef ENABLE_MOCKS


#include "functionshttptrigger.h"


static TEST_MUTEX_HANDLE g_testByTest;
static TEST_MUTEX_HANDLE g_dllByDll;

DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
	char temp_str[256];
	(void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
	ASSERT_FAIL(temp_str);
}


BEGIN_TEST_SUITE(functionshttptrigger_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
	TEST_INITIALIZE_MEMORY_DEBUG(g_dllByDll);
	g_testByTest = TEST_MUTEX_CREATE();
	ASSERT_IS_NOT_NULL(g_testByTest);

	umock_c_init(on_umock_c_error);

	REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
	REGISTER_GLOBAL_MOCK_FAIL_RETURN(gballoc_malloc, NULL);
	REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

	REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(MODULE_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(MESSAGE_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

	REGISTER_UMOCK_ALIAS_TYPE(HTTPAPI_REQUEST_TYPE, int);

	REGISTER_UMOCK_ALIAS_TYPE(HTTPAPIEX_RESULT, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
	umock_c_deinit();
	TEST_MUTEX_DESTROY(g_testByTest);
	TEST_DEINITIALIZE_MEMORY_DEBUG(g_dllByDll);


}

TEST_FUNCTION_INITIALIZE(method_init)
{
	if (TEST_MUTEX_ACQUIRE(g_testByTest))
	{
		ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
	}

	umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
	TEST_MUTEX_RELEASE(g_testByTest);
}


/* Tests_SRS_FUNCHTTPTRIGGER_04_020: [ Module_GetAPIS shall fill the provided MODULE_APIS function with the required function pointers. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Module_GetAPIS_returns_non_NULL)
{
	// arrange

	// act
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	//assert
	ASSERT_IS_TRUE(apis.Module_Destroy != NULL);
	ASSERT_IS_TRUE(apis.Module_Create != NULL);
	ASSERT_IS_TRUE(apis.Module_Receive != NULL);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_001: [ Upon success, this function shall return a valid pointer to a MODULE_HANDLE. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_happy_Path)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

  //act
	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42,  (const void*)&config);

	//assert
	ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup
	STRING_delete(config.relativePath);
	STRING_delete(config.hostAddress);
	apis.Module_Destroy(result);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_002: [ If the broker is NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_when_broker_is_NULL)
{
	// arrange
	MODULE_APIS apis;
    memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

    //act
	auto result = apis.Module_Create(NULL, (BROKER_HANDLE)0x42);

	//assert
	ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_003: [ If the configuration is NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_when_configuration_is_NULL)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);


	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, NULL);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_when_hostAddress_is_NULL)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = NULL;

	umock_c_reset_all_calls();

	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_004: [ If any hostAddress or relativePath are NULL, this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_when_relativePath_is_NULL)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = NULL;
	config.hostAddress = (STRING_HANDLE)0x42;

	umock_c_reset_all_calls();

	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_005: [ If FunctionsHttpTrigger_Create fails to allocate a new FUNCTIONS_HTTP_TRIGGER_DATA structure, then this function shall fail, and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_failed_To_Allocate_Handle)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;
	
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1)
		.SetReturn(NULL);

	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_005: [ If FunctionsHttpTrigger_Create fails to allocate a new FUNCTIONS_HTTP_TRIGGER_DATA structure, then this function shall fail, and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_failed_To_Allocate_configuration)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)))
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();


	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_006: [ If FunctionsHttpTrigger_Create fails to clone STRING for hostAddress, then this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_failed_to_clone_hostAddress)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();


	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_007: [ If FunctionsHttpTrigger_Create fails to clone STRING for relativePath, then this function shall fail and return NULL. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Create_returns_NULL_failed_to_clone_relativePath)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();


	//act
	auto result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);

	//assert
	ASSERT_IS_NULL(result);
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_008: [ If moduleHandle is NULL, FunctionsHttpTrigger_Destroy shall return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Destroy_does_nothing_if_module_handle_null)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	apis.Module_Destroy(NULL);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_009: [ FunctionsHttpTrigger_Destroy shall release all resources allocated for the module. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Destroy_happy_path)
{

	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE result = apis.Module_Create((BROKER_HANDLE)0x42, (const void*)&config);
	ASSERT_IS_NOT_NULL(result);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));
	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();

	STRICT_EXPECTED_CALL(gballoc_free(IGNORED_PTR_ARG))
		.IgnoreAllArguments();


	//act
	apis.Module_Destroy(result);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}



/* Tests_SRS_FUNCHTTPTRIGGER_04_010: [If moduleHandle is NULL than FunctionsHttpTrigger_Receive shall fail and return.] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_doesNothing_if_moduleHandleIsNull)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	//act
	apis.Module_Receive(NULL, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_011: [ If messageHandle is NULL than FunctionsHttpTrigger_Receive shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_doesNothing_if_messageHandleIsNull)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	//act
	apis.Module_Receive((MODULE_HANDLE)0x42, NULL);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_012: [ FunctionsHttpTrigger_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fails_when_Message_getContent_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(NULL);

	//act
	apis.Module_Receive((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_013: [ FunctionsHttpTrigger_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fails_when_Base64_Encode_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");
	
	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(buffer.buffer, buffer.size))
		.SetReturn(NULL);

	//act
	apis.Module_Receive((MODULE_HANDLE)0x42, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_014: [ FunctionsHttpTrigger_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_HTTPAPIEX_create_Fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");
	
	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_015: [ FunctionsHttpTrigger_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_if_buffer_new_fails)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_String_clone_fails)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn(NULL);

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_String_concat1_fails)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(-1);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_String_concat2_fails)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(-1);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_String_concat3_fails)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(-1);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_017: [ FunctionsHttpTrigger_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP GET to Azure Functions. If it fail it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_fail_when_httpapiex_executeRequest_fail)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest((HTTPAPIEX_HANDLE)0x42, HTTPAPI_REQUEST_GET, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG, NULL, (BUFFER_HANDLE)0x42))
		.IgnoreArgument(3)
		.IgnoreArgument(6)
		.SetReturn(HTTPAPIEX_ERROR);

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

/* Tests_SRS_FUNCHTTPTRIGGER_04_019: [ FunctionsHttpTrigger_Receive shall destroy any allocated memory before returning. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_018: [ Upon success FunctionsHttpTrigger_Receive shall log the response from HTTP GET and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_017: [ FunctionsHttpTrigger_Receive shall HTTPAPIEX_ExecuteRequest to send the HTTP GET to Azure Functions. If it fail it shall fail and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_016: [ FunctionsHttpTrigger_Receive shall add name and content parameter to relative path, if it fail it shall fail and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_015: [ FunctionsHttpTrigger_Receive shall call allocate memory to receive data from HTTPAPI by calling BUFFER_new, if it fail it shall fail and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_014: [ FunctionsHttpTrigger_Receive shall call HTTPAPIEX_Create, passing hostAddress, it if fails it shall fail and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_013: [ FunctionsHttpTrigger_Receive shall base64 encode by calling Base64_Encode_Bytes, if it fails it shall fail and return. ] */
/* Tests_SRS_FUNCHTTPTRIGGER_04_012: [ FunctionsHttpTrigger_Receive shall get the message content by calling Message_GetContent, if it fails it shall fail and return. ] */
TEST_FUNCTION(FUNCHTTPTRIGGER_Receive_happy_path)
{
	// arrange
	MODULE_APIS apis;
	memset(&apis, 0, sizeof(MODULE_APIS));
	Module_GetAPIS(&apis);

	CONSTBUFFER buffer;
	buffer.buffer = (const unsigned char*)"12345";
	buffer.size = sizeof("12345");

	FUNCTIONS_HTTP_TRIGGER_CONFIG config;
	config.relativePath = (STRING_HANDLE)0x42;
	config.hostAddress = (STRING_HANDLE)0x42;

	STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_NUM_ARG))
		.IgnoreArgument(1);

	STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(FUNCTIONS_HTTP_TRIGGER_CONFIG)));

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)IGNORED_PTR_ARG))
		.IgnoreArgument(1)
		.SetReturn((STRING_HANDLE)0x42);

	MODULE_HANDLE moduleInfo = apis.Module_Create((BROKER_HANDLE)0x42, &config);

	umock_c_reset_all_calls();

	STRICT_EXPECTED_CALL(Message_GetContent((MESSAGE_HANDLE)0x42))
		.SetReturn(&buffer);

	STRICT_EXPECTED_CALL(Base64_Encode_Bytes(IGNORED_PTR_ARG, IGNORED_NUM_ARG))
		.IgnoreAllArguments()
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_Create(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((HTTPAPIEX_HANDLE)0x42);

	STRICT_EXPECTED_CALL(BUFFER_new())
		.SetReturn((BUFFER_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_clone((STRING_HANDLE)0x42))
		.SetReturn((STRING_HANDLE)0x42);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(STRING_concat((STRING_HANDLE)0x42, IGNORED_PTR_ARG))
		.IgnoreArgument(2)
		.SetReturn(0);

	STRICT_EXPECTED_CALL(STRING_c_str(IGNORED_PTR_ARG))
		.IgnoreAllArguments()
		.SetReturn((const char*)"AnyContent42");

	STRICT_EXPECTED_CALL(HTTPAPIEX_ExecuteRequest((HTTPAPIEX_HANDLE)0x42, HTTPAPI_REQUEST_GET, IGNORED_PTR_ARG, NULL, NULL, IGNORED_PTR_ARG, NULL, (BUFFER_HANDLE)0x42))
		.IgnoreArgument(3)
		.IgnoreArgument(6)
		.SetReturn(HTTPAPIEX_OK);

	STRICT_EXPECTED_CALL(BUFFER_u_char((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	STRICT_EXPECTED_CALL(BUFFER_delete((BUFFER_HANDLE)0x42));

	STRICT_EXPECTED_CALL(HTTPAPIEX_Destroy((HTTPAPIEX_HANDLE)0x42));

	STRICT_EXPECTED_CALL(STRING_delete((STRING_HANDLE)0x42));

	//act
	apis.Module_Receive(moduleInfo, (MESSAGE_HANDLE)0x42);

	//assert
	ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

	//cleanup 
	apis.Module_Destroy(moduleInfo);
}

END_TEST_SUITE(functionshttptrigger_ut)
