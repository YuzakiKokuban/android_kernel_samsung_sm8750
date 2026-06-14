#ifndef __SEC_INPUT_KUNIT_MOCK_DUMMY_H__
#define __SEC_INPUT_KUNIT_MOCK_DUMMY_H__

#define REAL_ID(func_name) __real__##func_name
#define INVOKE_ID(func_name) __invoke__##func_name
#define RETURNS(return_type) return_type
#define __mockable
#define __mockable_alias(id) __alias(id)
#define __visible_for_testing static

#ifndef PARAMS
#define PARAMS(args...) args
#endif

#define KUNIT_EXPECT_STRNE(test, left, right) EXPECT_STRNE((struct test *)test, (left), (right))
#define EXPECT_STRNE(test, left, right) do {					\
	struct test_stream *__stream = EXPECT_START(test);			\
	typeof(left) __left = (left);						\
	typeof(right) __right = (right);					\
										\
	__stream->add(__stream, "Expected " #left " != " #right ", but\n");	\
	__stream->add(__stream, "\t\t%s == %s\n", #left, __left);		\
	__stream->add(__stream, "\t\t%s == %s\n", #right, __right);		\
										\
	EXPECT_END(test, strcmp(left, right), __stream);			\
} while (0)

#define DECLARE_REDIRECT_MOCKABLE(name, return_type, param_types...)	\
	return_type name(param_types)

#define DEFINE_INVOKABLE(name, return_type, RETURN_ASSIGN, param_types...)

#define DEFINE_REDIRECT_MOCKABLE_COMMON(name,			\
		return_type,					\
		RETURN_ASSIGN,					\
		param_types...)					\
		return_type REAL_ID(name)(param_types);		\
		return_type name(param_types) __mockable_alias(REAL_ID(name));	\
		DEFINE_INVOKABLE(name, return_type, RETURN_ASSIGN, param_types);

#define ASSIGN() *retval =

#define DEFINE_REDIRECT_MOCKABLE(name, return_type, param_types...)	\
	DEFINE_REDIRECT_MOCKABLE_COMMON(name,				\
			return_type,					\
			ASSIGN,						\
			param_types)

#endif /* __SEC_INPUT_KUNIT_MOCK_DUMMY_H__ */
