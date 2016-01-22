/**
 * Copyright (C) 2013-2015 ECNU_DaSE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * @file  ob_schema_service.cpp
 * @brief 表单schema相关数据结构。创建，删除，获取schema描述结构接口
 *
 * modified by longfei：
 * 1.add two more member variables in TableSchema(struct) and their serialize() series function
 * 2.add IndexBeat for construct static secondary index CS <==heart beat==> RS
 *
 * @version __DaSE_VERSION
 * @author longfei <longfei@stu.ecnu.edu.cn>
 * @date 2016_01_21
 */
#include "ob_schema_service.h"
#include "common/utility.h"
#include "common/ob_common_param.h"
#include "common/serialization.h"
using namespace oceanbase::common;

bool TableSchema::is_valid() const
{
  int err = OB_SUCCESS;
  bool ret = false;
  if ('\0' == table_name_[0])
  {
    TBSYS_LOG(WARN, "table name is empty");
  }
  else if (OB_INVALID_ID == table_id_
           || OB_NOT_EXIST_TABLE_TID == table_id_
           || OB_FIRST_META_VIRTUAL_TID == table_id_)
  {
    TBSYS_LOG(WARN, "invalid table id=%lu", table_id_);
  }
  else if (1 > rowkey_column_num_
           || OB_MAX_ROWKEY_COLUMN_NUMBER < rowkey_column_num_)
  {
    TBSYS_LOG(USER_ERROR, "no primary key specified:table_name[%s]", table_name_);
  }
  else if (1 > replica_num_
           || OB_TABLET_MAX_REPLICA_COUNT < replica_num_)
  {
    TBSYS_LOG(USER_ERROR, "invalid replica num %d", replica_num_);
  }
  else
  {
    int32_t def_rowkey_col = 0;
    for (int32_t i = 0; i < columns_.count(); ++i)
    {
      if (columns_.at(i).rowkey_id_ != 0)
      {
        ++def_rowkey_col;
        if (ObCreateTimeType == columns_.at(i).data_type_ || ObModifyTimeType == columns_.at(i).data_type_)
        {
          TBSYS_LOG(USER_ERROR, "column '%s' with %s type as primary key not support",  columns_.at(i).column_name_,
              (ObCreateTimeType == columns_.at(i).data_type_) ? "createtime" : "modifytime");
          err = OB_ERR_INVALID_SCHEMA;
          break;
        }
        if (static_cast<int64_t>(columns_.at(i).column_id_) > max_used_column_id_)
        {
          TBSYS_LOG(USER_ERROR, "column id is greater than max_used_column_id, name=%s id=%ld max=%ld",
              columns_.at(i).column_name_, columns_.at(i).column_id_, max_used_column_id_);
          err = OB_ERR_INVALID_SCHEMA;
          break;
        }
      }
    }
    if (OB_SUCCESS == err)
    {
      if (def_rowkey_col == columns_.count())
      {
        TBSYS_LOG(WARN, "all columns=%ld are defined as rowkey column=%d, table=%s",
            columns_.count(), def_rowkey_col, table_name_);
      }
      else if (def_rowkey_col == rowkey_column_num_)
      {
        ret = true;
      }
      else
      {
        TBSYS_LOG(WARN, "rowkey_column_num=%d but defined_num=%d, table=%s",
            rowkey_column_num_, def_rowkey_col, table_name_);
      }
    }
  }
  return ret;
}

int TableSchema::to_string(char* buf, const int64_t buf_len) const
{
  int ret = OB_SUCCESS;
  int64_t pos = 0;
  databuff_printf(buf, buf_len, pos,
                  "tname=%s "
                  "tid=%lu "
                  "table_type=%d "
                  "load_type=%d "
                  "table_def_type=%d "
                  "rowkey_column_num=%d "
                  "replica_num=%d "
                  "max_used_column_id=%ld "
                  "create_mem_version=%ld "
                  "tablet_block_size=%ld "
                  "tablet_max_size=%ld "
                  "max_rowkey_length=%ld "
                  "consistency_level=%s",
                  table_name_,
                  table_id_,
                  table_type_,
                  load_type_,
                  table_def_type_,
                  rowkey_column_num_,
                  replica_num_,
                  max_used_column_id_,
                  create_mem_version_,
                  tablet_block_size_,
                  tablet_max_size_,
                  max_rowkey_length_,
                  (consistency_level_ == common::STATIC) ? "STATIC" : (consistency_level_ == common::STRONG ? "STRONG" : (consistency_level_ == common::WEAK ? "WEAK" : "FROZEN")));
  for (int64_t i = 0; i < columns_.count(); ++i)
  {
    const ColumnSchema &tcolumn = columns_.at(i);
    databuff_printf(buf, buf_len, pos, "<column=%ld cname=%s cid=%lu data_type=%d rowkey_id=%ld> ",
                    i, tcolumn.column_name_,
                    tcolumn.column_id_,
                    tcolumn.data_type_,
                    tcolumn.rowkey_id_);
  }
  return ret;
}

void TableSchema::clear()
{
  table_id_ = OB_INVALID_ID;
  table_name_[0] = '\0';
  columns_.clear();
  join_info_.clear();
}

DEFINE_SERIALIZE(ColumnSchema)
{
  int ret = OB_SUCCESS;
  if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, column_name_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, column_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, column_group_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, rowkey_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, join_table_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, join_column_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, data_type_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, data_length_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, data_precision_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, data_scale_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_bool(buf, buf_len, pos, nullable_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, length_in_rowkey_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, gm_create_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, gm_modify_)))
  {
  }
  return ret;
}

DEFINE_DESERIALIZE(ColumnSchema)
{
  int ret = OB_SUCCESS;
  int64_t len = 0;
  serialization::decode_vstr(buf, data_len, pos, column_name_, OB_MAX_COLUMN_NAME_LENGTH, &len);
  if (len < 0)
  {
    ret = OB_SERIALIZE_ERROR;
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&column_id_))))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&column_group_id_))))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &rowkey_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&join_table_id_))))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&join_column_id_))))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, reinterpret_cast<int32_t*>(&data_type_))))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &data_length_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &data_precision_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &data_scale_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_bool(buf, data_len, pos, &nullable_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &length_in_rowkey_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &gm_create_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &gm_modify_)))
  {
  }
  return ret;
}

DEFINE_SERIALIZE(TableSchema)
{
  int ret = OB_SUCCESS;
  if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, table_name_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, compress_func_name_)))
  {
  }
  // expire_info is deprecated
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, expire_condition_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, comment_str_)))
  {

  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, table_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, table_type_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, load_type_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, table_def_type_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_bool(buf, buf_len, pos, is_use_bloomfilter_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_bool(buf, buf_len, pos, is_pure_update_table_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, consistency_level_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, rowkey_split_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, rowkey_column_num_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, replica_num_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, max_used_column_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, create_mem_version_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, tablet_block_size_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, tablet_max_size_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, max_rowkey_length_)))
  {
  }

  // longfei [create index]
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, original_table_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi32(buf, buf_len, pos, index_status_)))
  {
  }

  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, merge_write_sstable_version_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, schema_version_)))
  {

  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, create_time_column_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, modify_time_column_id_)))
  {
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, columns_.count())))
  {
  }
  else
  {
    for (int64_t i = 0; OB_SUCCESS == ret && i < columns_.count(); ++i)
    {
      if (OB_SUCCESS != (ret = columns_.at(i).serialize(buf, buf_len, pos)))
      {
        TBSYS_LOG(WARN, "failed to serialize column, err=%d", ret);
        break;
      }
    }
  }

  if (OB_SUCCESS == ret)
  {
    if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, join_info_.count())))
    {
      TBSYS_LOG(WARN, "fail to serialize join info count:ret[%d]", ret);
    }
  }
  if (OB_SUCCESS == ret)
  {
    for (int64_t i = 0; OB_SUCCESS == ret && i < join_info_.count(); i ++)
    {
      if (OB_SUCCESS != (ret = join_info_.at(i).serialize(buf, buf_len, pos)))
      {
        TBSYS_LOG(WARN, "fail to serialization join info:ret[%d], i[%ld]", ret, i);
      }
    }
  }
  return ret;
}

DEFINE_DESERIALIZE(TableSchema)
{
  int ret = OB_SUCCESS;
  int64_t len1 = 0;
  int64_t len2 = 0;
  int64_t len3 = 0;
  int64_t len4 = 0;
  int64_t column_count = 0;
  serialization::decode_vstr(buf, data_len, pos, table_name_, OB_MAX_TABLE_NAME_LENGTH, &len1);
  serialization::decode_vstr(buf, data_len, pos, compress_func_name_, OB_MAX_TABLE_NAME_LENGTH, &len2);
  serialization::decode_vstr(buf, data_len, pos, expire_condition_, OB_MAX_EXPIRE_CONDITION_LENGTH, &len3);
  serialization::decode_vstr(buf, data_len, pos, comment_str_, OB_MAX_TABLE_COMMENT_LENGTH, &len4);
  if (len1 < 0 || len2 < 0 || len3 < 0 || len4 < 0)
  {
    TBSYS_LOG(WARN, "deserialize error, len1=%ld len2=%ld len3=%ld len4=%ld", len1, len2, len3, len4);
    ret = OB_DESERIALIZE_ERROR;
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&table_id_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, reinterpret_cast<int32_t*>(&table_type_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, reinterpret_cast<int32_t*>(&load_type_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, reinterpret_cast<int32_t*>(&table_def_type_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_bool(buf, data_len, pos, &is_use_bloomfilter_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_bool(buf, data_len, pos, &is_pure_update_table_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &consistency_level_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &rowkey_split_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, &rowkey_column_num_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, &replica_num_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &max_used_column_id_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &create_mem_version_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &tablet_block_size_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &tablet_max_size_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &max_rowkey_length_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }

  // longfei [create index]
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&original_table_id_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi32(buf, data_len, pos, reinterpret_cast<int32_t*>(&index_status_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }

  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &merge_write_sstable_version_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &schema_version_)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&create_time_column_id_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&modify_time_column_id_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &column_count)))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else
  {
    ColumnSchema col_schema;
    for (int64_t i = 0; i < column_count; ++i)
    {
      if (OB_SUCCESS != (ret = col_schema.deserialize(buf, data_len, pos)))
      {
        TBSYS_LOG(WARN, "failed to deserialize column, err=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = columns_.push_back(col_schema)))
      {
        TBSYS_LOG(WARN, "failed to push into array, err=%d", ret);
        break;
      }
    }
  }

  int64_t join_info_count = 0;
  if (OB_SUCCESS == ret)
  {
    if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &join_info_count)))
    {
      TBSYS_LOG(WARN, "fail to deserialize join info count:ret[%d]", ret);
    }
  }
  if (OB_SUCCESS == ret)
  {
    join_info_.clear();
    JoinInfo join_info;
    for (int64_t i = 0; OB_SUCCESS == ret && i < join_info_count; i ++)
    {
      if (OB_SUCCESS != (ret = join_info.deserialize(buf, data_len, pos)))
      {
        TBSYS_LOG(WARN, "fail to deserialize join info:ret[%d]", ret);
      }
      else if (OB_SUCCESS != (ret = join_info_.push_back(join_info)))
      {
        TBSYS_LOG(WARN, "fail to push join info to array;ret[%d]", ret);
      }
    }
  }
  return ret;
}

DEFINE_SERIALIZE(AlterTableSchema)
{
  int ret = OB_SUCCESS;
  if ('\0' == table_name_[0])
  {
    ret = OB_INVALID_ARGUMENT;
    TBSYS_LOG(WARN, "table name is empty");
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, table_name_)))
  {
    TBSYS_LOG(WARN, "failed to serialize table name, err=%d", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, columns_.count())))
  {
    TBSYS_LOG(WARN, "failed to serialize column count, err=%d", ret);
  }
  else
  {
    for (int64_t i = 0; i < columns_.count(); ++i)
    {
      if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, columns_.at(i).type_)))
      {
        TBSYS_LOG(WARN, "failed to serialize is add flag, err=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = columns_.at(i).column_.serialize(buf, buf_len, pos)))
      {
        TBSYS_LOG(WARN, "failed to serialize column, err=%d", ret);
        break;
      }
    }
  }
  return ret;
}

DEFINE_SERIALIZE(JoinInfo)
{
  int ret = OB_SUCCESS;
  if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, left_table_name_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, left_table_id_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, left_column_name_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, left_column_id_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, right_table_name_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, right_table_id_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vstr(buf, buf_len, pos, right_column_name_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, right_column_id_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  return ret;
}

DEFINE_DESERIALIZE(JoinInfo)
{
  int ret = OB_SUCCESS;
  int64_t len = 0;
  int64_t id = 0;
  if (OB_SUCCESS == ret)
  {
    serialization::decode_vstr(buf, data_len, pos, left_table_name_, OB_MAX_TABLE_NAME_LENGTH, &len);
    if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &id)))
    {
      TBSYS_LOG(WARN, "fail to deserialize:ret[%d]", ret);
    }
    else
    {
      left_table_id_ = id;
    }
  }

  if (OB_SUCCESS == ret)
  {
    serialization::decode_vstr(buf, data_len, pos, left_column_name_, OB_MAX_COLUMN_NAME_LENGTH, &len);
    if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &id)))
    {
      TBSYS_LOG(WARN, "fail to deserialize:ret[%d]", ret);
    }
    else
    {
      left_column_id_ = id;
    }
  }

  if (OB_SUCCESS == ret)
  {
    serialization::decode_vstr(buf, data_len, pos, right_table_name_, OB_MAX_TABLE_NAME_LENGTH, &len);
    if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &id)))
    {
      TBSYS_LOG(WARN, "fail to deserialize:ret[%d]", ret);
    }
    else
    {
      right_table_id_ = id;
    }
  }

  if (OB_SUCCESS == ret)
  {
    serialization::decode_vstr(buf, data_len, pos, right_column_name_, OB_MAX_COLUMN_NAME_LENGTH, &len);
    if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &id)))
    {
      TBSYS_LOG(WARN, "fail to deserialize:ret[%d]", ret);
    }
    else
    {
      right_column_id_ = id;
    }
  }
  return ret;
}

DEFINE_DESERIALIZE(AlterTableSchema)
{
  int ret = OB_SUCCESS;
  int64_t table_name_len = 0;
  int64_t column_count = 0;
  serialization::decode_vstr(buf, data_len, pos, table_name_, OB_MAX_TABLE_NAME_LENGTH, &table_name_len);
  if (table_name_len <= 0)
  {
    ret = OB_INVALID_ARGUMENT;
    TBSYS_LOG(WARN, "check table name len failed, err=%d", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &column_count)))
  {
    TBSYS_LOG(WARN, "failed to deserialize column count, err=%d", ret);
  }
  else
  {
    int64_t alter_type;
    ColumnSchema alter_column;
    for (int64_t i = 0; i < column_count; ++i)
    {
      if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, &alter_type)))
      {
        TBSYS_LOG(WARN, "failed to deserialize is add flag, err=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = alter_column.deserialize(buf, data_len, pos)))
      {
        TBSYS_LOG(WARN, "failed to deserialize column, err=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = add_column(AlterType(alter_type), alter_column)))
      {
        TBSYS_LOG(WARN, "failed to add column, err=%d", ret);
        break;
      }
    }
  }
  return ret;
}

//add longfei [cons static index] 151120:b
DEFINE_SERIALIZE(IndexBeat)
{
  int ret=OB_SUCCESS;
  if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, idx_tid_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_vi64(buf, buf_len, pos, hist_width_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_i32(buf, buf_len, pos, status_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }
  else if (OB_SUCCESS != (ret = serialization::encode_i32(buf, buf_len, pos, stage_)))
  {
    TBSYS_LOG(WARN, "fail to serialize:ret[%d]", ret);
  }

  return ret;
}

DEFINE_DESERIALIZE(IndexBeat)
{
  int ret = OB_SUCCESS;
  int32_t status;
  int32_t stage;
  if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&idx_tid_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else  if (OB_SUCCESS != (ret = serialization::decode_vi64(buf, data_len, pos, reinterpret_cast<int64_t*>(&hist_width_))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else  if (OB_SUCCESS != (ret = serialization::decode_i32(buf, data_len, pos, reinterpret_cast<int32_t*>(&status))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else  if (OB_SUCCESS != (ret = serialization::decode_i32(buf, data_len, pos, reinterpret_cast<int32_t*>(&stage))))
  {
    TBSYS_LOG(WARN, "deserialize error here");
  }
  else
  {
    if(0 == status)
    {
      status_ = NOT_AVALIBALE;
    }
    else if(1 == status)
    {
      status_ = AVALIBALE;
    }
    else if(3 == status)
    {
      status_ = WRITE_ONLY;
    }
    else if(4 == status)
    {
      status_ = INDEX_INIT;
    }
    else
    {
      status_ = ERROR;
    }

    if (0 == stage)
    {
      stage_ = LOCAL_INDEX_STAGE;
    }
    else if (1 == stage)
    {
      stage_ = GLOBAL_INDEX_STAGE;
    }
  }
  return ret;
}
// add e

////add maoxx
//int ObSchemaService::check_column_checksum(const int64_t orginal_table_id, const int64_t index_table_id, const int64_t cluster_id, const int64_t current_version, bool &column_checksum_flag)
//{
//    int ret = OB_SUCCESS;
//    QueryRes* res = NULL;
//    TableRow* table_row = NULL;
//    ObNewRange range;
//    int64_t cell_index = 0;//因为只取出column checksum列的数据，所以为0
//    int64_t rowkey_column_num = 4;
//    ObObj start_rowkey[rowkey_column_num];
//    ObObj end_rowkey[rowkey_column_num];
//    ObColumnChecksum original_table_column_checksum;
//    ObColumnChecksum index_table_column_checksum;
//    ObColumnChecksum row_column_checksum;

//    ObString tmp_string;
//    char tmp_char[OB_MAX_COL_CHECKSUM_STR_LEN];
//    tmp_string.assign_ptr(tmp_char, OB_MAX_COL_CHECKSUM_STR_LEN);

//    original_table_column_checksum.reset();
//    index_table_column_checksum.reset();

//    start_rowkey[0].set_int(orginal_table_id);
//    end_rowkey[0].set_int(orginal_table_id);
//    start_rowkey[1].set_int(cluster_id);
//    end_rowkey[1].set_int(cluster_id);
//    start_rowkey[2].set_int(current_version);
//    end_rowkey[2].set_int(current_version);
//    start_rowkey[3].set_min_value();
//    end_rowkey[3].set_max_value();

//    range.start_key_.assign(start_rowkey, rowkey_column_num);
//    range.end_key_.assign(end_rowkey, rowkey_column_num);
//    range.border_flag_.inclusive_end();
//    range.border_flag_.inclusive_start();

//    if(OB_SUCCESS != (ret = nb_accessor_.scan(res, OB_ALL_COLUMN_CHECKSUM_INFO_TABLE_NAME, range, SC("column_checksum"))))
//    {
//      TBSYS_LOG(WARN, "fail to nb scan column checksum info of orginal table %d", ret);
//    }
//    else
//    {
//      TBSYS_LOG(INFO, "start calculate column checksum of orginal_table_id:%ld", orginal_table_id);
//      while(OB_SUCCESS == (ret = res->next_row()))
//      {
//        if(OB_SUCCESS != (ret = res->get_row(&table_row)))
//        {
//          TBSYS_LOG(ERROR, "failed to get next row of column checksum");
//          break;
//        }
//        else if(NULL != table_row)
//        {
//          if(OB_SUCCESS != (ret = table_row->get_cell_info(cell_index)->value_.get_varchar(tmp_string)))
//          {
//            TBSYS_LOG(ERROR, "failed calculate column checksum of orginal_table_id:%ld", orginal_table_id);
//            break;
//          }
//          else
//          {
//            row_column_checksum.deepcopy(tmp_string.ptr(),(int32_t)tmp_string.length());
//            ret = original_table_column_checksum.sum(row_column_checksum);
//            row_column_checksum.reset();
//            tmp_string.reset();
//          }
//        }
//        else
//        {
//          ret = OB_ERROR;
//          TBSYS_LOG(ERROR, "failed calculate column checksum of orginal_table_id:%ld", orginal_table_id);
//          break;
//        }
//      }
//      if (OB_ITER_END != ret)
//      {
//        TBSYS_LOG(WARN, "failed to get index table column checksum tabe_id:%ld", orginal_table_id);
//      }
//      else
//      {
//          ret = OB_SUCCESS;
//      }
//    }

//    if(OB_SUCCESS == ret)
//    {
//      start_rowkey[0].set_int(index_table_id);
//      end_rowkey[0].set_int(index_table_id);
//      start_rowkey[1].set_int(cluster_id);
//      end_rowkey[1].set_int(cluster_id);
//      start_rowkey[2].set_int(current_version);
//      end_rowkey[2].set_int(current_version);
//      start_rowkey[3].set_min_value();
//      end_rowkey[3].set_max_value();

//      range.start_key_.assign(start_rowkey, rowkey_column_num);
//      range.end_key_.assign(end_rowkey, rowkey_column_num);
//      range.border_flag_.inclusive_end();
//      range.border_flag_.inclusive_start();

//      nb_accessor_.release_query_res(res);
//      res = NULL;

//      if(OB_SUCCESS != (ret = nb_accessor_.scan(res, OB_ALL_COLUMN_CHECKSUM_INFO_TABLE_NAME, range, SC("column_checksum"))))
//      {
//        TBSYS_LOG(WARN, "fail to nb scan column checksum info of index table %d", ret);
//      }
//      else
//      {
//        TBSYS_LOG(INFO, "start calculate column checksum of index_table_id:%ld", index_table_id);
//        while(OB_SUCCESS == (ret = res->next_row()))
//        {
//          if(OB_SUCCESS != (ret = res->get_row(&table_row)))
//          {
//            TBSYS_LOG(ERROR, "failed to get next row of column checksum");
//            break;
//          }
//          else if(NULL != table_row)
//          {
//            if(OB_SUCCESS != (ret = table_row->get_cell_info(cell_index)->value_.get_varchar(tmp_string)))
//            {
//              TBSYS_LOG(ERROR, "failed calculate column checksum of index_table_id:%ld", index_table_id);
//              break;
//            }
//            else
//            {
//              row_column_checksum.deepcopy(tmp_string.ptr(),(int32_t)tmp_string.length());
//              ret = index_table_column_checksum.sum(row_column_checksum);
//              row_column_checksum.reset();
//              tmp_string.reset();
//            }
//          }
//          else
//          {
//            ret = OB_ERROR;
//            TBSYS_LOG(WARN, "failed calculate column checksum of index_table_id:%ld", index_table_id);
//            break;
//          }
//        }
//        if (OB_ITER_END != ret)
//        {
//          TBSYS_LOG(WARN, "failed to get index table column checksum tabe_id:%ld", index_table_id);
//        }
//        else
//        {
//          ret = OB_SUCCESS;
//        }
//      }
//    }

//    if (OB_SUCCESS == ret)
//    {
//      if(original_table_column_checksum.compare(index_table_column_checksum))
//      {
//        column_checksum_flag = true;
//        TBSYS_LOG(INFO, "this index table column checksum is correct table_id:%ld", index_table_id);
//      }
//      else
//      {
//        column_checksum_flag = false;
//        TBSYS_LOG(WARN, "this index table column checksum is incorrect table_id:%ld", index_table_id);
//      }
//    }

//    return ret;
//}

//int ObSchemaServiceImpl::clean_column_checksum(const int64_t max_draution_of_version, const int64_t current_version)
//{
//    int ret = OB_SUCCESS;
//    QueryRes* res = NULL;
//    TableRow* table_row = NULL;
//    ObNewRange range;
//    int64_t cell_index = 0;

//    range.set_whole_range();

//    TBSYS_LOG(INFO, "clean version less than %ld", current_version - max_draution_of_version);

//    if(OB_SUCCESS != (ret = nb_accessor_.scan(res, OB_ALL_COLUMN_CHECKSUM_INFO_TABLE_NAME, range, SC("version"), ScanConds("version", LT, current_version - max_draution_of_version))))
//    {
//      TBSYS_LOG(WARN, "failed to scan data to delete from column_checksum ret[%d]" , ret);
//    }
//    else
//    {
//      while(OB_SUCCESS == res->next_row())
//      {
//        if(OB_SUCCESS != (ret = res->get_row(&table_row)))
//        {
//          TBSYS_LOG(ERROR, "failed to get next row of column checksum");
//          break;
//        }
//        else if(NULL != table_row)
//        {
//          if(OB_SUCCESS != (ret = nb_accessor_.delete_row(OB_ALL_COLUMN_CHECKSUM_INFO_TABLE_NAME, table_row->get_cell_info(cell_index)->row_key_)))
//          {
//            TBSYS_LOG(WARN, "failed to delete one row from column cheksum ret[%d]", ret);
//            break;
//          }
//        }
//        else
//        {
//          ret = OB_ERROR;
//          TBSYS_LOG(WARN, "delete column checksum row is NULL");
//          break;
//        }
//      }
//      if (OB_ITER_END != ret)
//      {
//        TBSYS_LOG(WARN, "failed to get table column checksum");
//      }
//      else
//      {
//        ret = OB_SUCCESS;
//      }
//    }
//    return ret;
//}

//int ObSchemaServiceImpl::get_column_checksum(const ObNewRange range, const int64_t cluster_id, const int64_t required_version, ObString& column_checksum)
//{
//  int ret = OB_SUCCESS;
//  QueryRes* res = NULL;
//  TableRow* table_row = NULL;
//  ObRowkey rowkey;
//  ObObj rowkey_list[4];

//  char range_buf[OB_RANGE_STR_BUFSIZ];
//  range.to_string(range_buf, sizeof(range_buf));
//  int32_t len = static_cast<int32_t>(strlen(range_buf));
//  ObString str_range(0, len, range_buf);

//  rowkey_list[0].set_int(range.table_id_);
//  rowkey_list[1].set_int(cluster_id);
//  rowkey_list[2].set_int(required_version);
//  rowkey_list[3].set_varchar(str_range);
//  rowkey.assign(rowkey_list,4);

//  char varchar_column_checksum[common::OB_MAX_COL_CHECKSUM_STR_LEN];
//  if(OB_SUCCESS != ( ret = nb_accessor_.get(res, OB_ALL_COLUMN_CHECKSUM_INFO_TABLE_NAME, rowkey, SC("column_checksum"))))
//  {
//    TBSYS_LOG(ERROR, "faild to get nb column checksum ret = %d", ret);
//  }
//  else
//  {
//    table_row = res->get_only_one_row();
//    if(NULL != table_row)
//    {
//      ASSIGN_VARCHAR("column_checksum", varchar_column_checksum, common::OB_MAX_COL_CHECKSUM_STR_LEN);
//      len = static_cast<int32_t>(strlen(varchar_column_checksum));
//      column_checksum.write(varchar_column_checksum, len);
//    }
//    else
//    {
//      ret = OB_ENTRY_NOT_EXIST;
//      TBSYS_LOG(WARN, "get table row fail ret = %d", ret);
//    }
//  }
//  memset(range_buf, 0, OB_RANGE_STR_BUFSIZ*sizeof(char));
//  return ret;
//}
////add e
