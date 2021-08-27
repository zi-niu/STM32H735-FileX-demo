#ifndef __UTILS_MICRO_TEST_H_
#define __UTILS_MICRO_TEST_H_

#include <stdio.h>

#define true	1
#define false	0

int did_test_fail = false;

/* 可以再添加一个参数，传入函数指针，指针不为null时，发生错误执行该函数 */

#define MICRO_EXPECT(x)                               						\
  do {                                                        				\
    if (!(x)) {                                               				\
      printf(#x " failed at %s:%d\r\n", __FILE__, __LINE__); 				\
      did_test_fail = true;                       							\
    }                                                         				\
  } while (false)

/*
 * 定义临时变量vx vy用于防止传入参数x,y为函数时，函数在判断和输出语句中分别执行一次
 */
#define MICRO_EXPECT_EQ(x, y)                                    			\
  do {                                                                   	\
	int vx = (int)x;														\
	int vy = (int)y;														\
    if ((vx) != (vy)) {                                                  	\
      printf(#x " == " #y " failed at %s:%d (%d vs %d)\r\n", __FILE__,  	\
                  __LINE__, (int)(vx), (int)(vy)); 							\
      did_test_fail = true;                                  				\
    }                                                                    	\
  } while (false)

#define MICRO_EXPECT_NE(x, y)                                   			\
  do {                                                                  	\
    if ((x) == (y)) {                                                   	\
      printf(#x " != " #y " failed at %s:%d\r\n", __FILE__, __LINE__); 		\
      did_test_fail = true;                                 				\
    }                                                                   	\
  } while (false)

#define MICRO_EXPECT_TRUE(x)                                       			\
  do {                                                                     	\
    if (!(x)) {                                                            	\
      printf(#x " was not true failed at %s:%d\r\n", __FILE__, __LINE__); 	\
      did_test_fail = true;                                    				\
    }                                                                      	\
  } while (false)

#define MICRO_EXPECT_FALSE(x)                                       		\
  do {                                                                      \
    if (x) {                                                                \
      printf(#x " was not false failed at %s:%d\r\n", __FILE__, __LINE__); 	\
      did_test_fail = true;                                     			\
    }                                                                       \
  } while (false)

#define MICRO_FAIL(msg)                       								\
  do {                                              						\
    printf("FAIL: %s at %s:%d\r\n", msg, __FILE__, __LINE__); 				\
    did_test_fail = true;                 									\
  } while (false)

#define MICRO_MARK															\
  do {                                              						\
	printf("[mark]: at %s:%d\r\n", __FILE__, __LINE__); 					\
  } while (false)

#endif  // TENSORFLOW_LITE_MICRO_TESTING_MICRO_TEST_H_
