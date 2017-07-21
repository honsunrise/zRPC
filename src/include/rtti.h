//
// Created by zhsyourai on 11/30/16.
//

#ifndef ZRPC_RTTI_H
#define ZRPC_RTTI_H

/**
 * Please let your struct first statement is DECLARE_RTTI;
 * example:
 *  struct type {
 *      DECLARE_RTTI();
 *      other...;
 *  };
 *
 */

struct __dummy_struct {
  const char *_rtti_flag_;
};

#define DECLARE_RTTI \
    const char *_rtti_flag_

#define RTTI_INIT(type_name, val) \
    (val)._rtti_flag_ = #type_name

#define RTTI_INIT_PTR(type_name, val) \
    (val)->_rtti_flag_ = #type_name

#define RTTI_TYPEID_PTR(type_name, val) \
    (strcmp(((struct __dummy_struct*) val)->_rtti_flag_, #type_name ) == 0)

#define IF_TYPE_SAME(type_name, val) \
    if(RTTI_TYPEID_PTR(type_name, val))

#define ELSE_IF_TYPE_SAME(type_name, val) \
    else if(RTTI_TYPEID_PTR(type_name, val))

#endif //ZRPC_RTTI_H
