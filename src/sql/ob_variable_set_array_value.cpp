#include "ob_variable_set_array_value.h"
#include "ob_phy_operator.h"
#include "ob_sql_session_info.h"
namespace oceanbase
{
  namespace sql
  {
    ObVariableSetArrayValue::ObVariableSetArrayValue()
    {
    }

    ObVariableSetArrayValue::~ObVariableSetArrayValue()
    {
      values_.clear();
    }

    int ObVariableSetArrayValue::open()
    {
      ObSQLSessionInfo *session = my_phy_plan_->get_result_set()->get_session();

      int ret = OB_SUCCESS;

      if( OB_SUCCESS != (ret = session->replace_vararray(var_name_, values_)))
      {
        TBSYS_LOG(WARN, "replace array variables fail");
      }

      return ret;
    }

    int64_t ObVariableSetArrayValue::to_string(char *buf, const int64_t buf_len) const
    {
      int64_t pos = 0;
      databuff_printf(buf, buf_len, pos, "VariableSetArrayValue( %.*s = [", var_name_.length(), var_name_.ptr());
      for(int64_t i = 0; i < values_.count(); ++i)
      {
        pos += values_.at(i).to_string(buf + pos, buf_len - pos);
        databuff_printf(buf, buf_len, pos, " ");
      }
      databuff_printf(buf, buf_len, pos, "]");
      return pos;
    }
  }
}

namespace oceanbase{
  namespace sql{
    REGISTER_PHY_OPERATOR(ObVariableSetArrayValue, PHY_VARIABLE_SET_ARRAY);
  }
}
