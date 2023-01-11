//
// Created by lining on 9/30/22.
//

#ifndef _MATRIXCONTROLCOM_H
#define _COM_H

#define ARRAY_SIZE(x) \
    (sizeof(x)/sizeof(x[0]))
#define OFFSET(type, member)      \
    ( (size_t)( &( ((type*)0)->member)  ) )
#define MEMBER_SIZE(type, member) \
    sizeof(((type *)0)->member)


#endif //_MATRIXCONTROLCOM_H
