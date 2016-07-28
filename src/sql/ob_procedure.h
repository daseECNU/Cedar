/**
* Copyright (C) 2013-2016 ECNU_DaSE.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* @file ob_procedure.h
* @brief procedure phsical plan relation class definition
*
* Created by zhutao
*
* @version __DaSE_VERSION
* @author zhutao <zhutao@stu.ecnu.edu.cn>
* @author wangdonghui <zjnuwangdonghui@163.com>
* @date 2016_07_27
*/

#ifndef OCEANBASE_SQL_OB_PROCEDURE_H
#define OCEANBASE_SQL_OB_PROCEDURE_H
#include "ob_sp_procedure.h"
#include "ob_no_children_phy_operator.h"
#include "ob_sql_session_info.h"
#include "common/dlist.h"
#include "ob_procedure_stmt.h"
#include "ob_procedure_assgin_stmt.h"
#include "ob_procedure_declare_stmt.h"
#include "ob_raw_expr.h"
#include "mergeserver/ob_ms_rpc_proxy.h"
using namespace oceanbase::common;

namespace oceanbase
{
	namespace sql
	{
    class ObPhysicalPlan;
    class ObProcedure;
    class SpProcedure;
    class ObProcedureOptimizer;
    /**
     * @brief The SpMsInstExecStrategy class
     * MS instruction execution strategy
     */
    class SpMsInstExecStrategy : public SpInstExecStrategy
    {
    public:
      /**
       * @brief execute_inst
       * execute a instruction
       * @param inst instruction need be execute
       * @return error code
       */
      virtual int execute_inst(SpInst *inst); //provide simple routine for inst execution
      /**
       * @brief hkey
       * return hkey that distinguish base line data generated by
       * the same instruction in different loop iterations
       * @param sdata_id  static data id
       * @return hkey
       */
      int64_t hkey(int64_t sdata_id) const;

    private:
      /**
       * @brief execute_expr
       * execute expression instruction
       * @param inst expression instruction
       * @return error code
       */
      virtual int execute_expr(SpExprInst *inst);
      /**
       * @brief execute_rd_base
       * execute read base data instruction
       * @param inst read base data instruction
       * @return error code
       */
      virtual int execute_rd_base(SpRdBaseInst *inst);
      /**
       * @brief execute_wr_delta
       * execute write delta data instruction by send ups
       * @param inst wr delta data instruction
       * @return error code
       */
      virtual int execute_wr_delta(SpRwDeltaInst *inst);
      /**
       * @brief execute_rd_delta
       * execute read delta data and write to variable instruction
       * @param inst read delta data instruction
       * @return error code
       */
      virtual int execute_rd_delta(SpRwDeltaIntoVarInst *inst);
      /**
       * @brief execute_rw_all
       * execute read and write delta data instruction
       * @param inst read and write delta data instruction
       * @return error code
       */
      virtual int execute_rw_all(SpRwCompInst *inst);


      virtual int execute_plain_sql(SpPlainSQLInst *inst);

      /**
       * @brief execute_group
       * group instructions execution
       * @param inst group instructions
       * @return error code
       */
      virtual int execute_group(SpGroupInsts *inst);
      /**
       * @brief execute_if_ctrl
       * execute if control instruction
       * @param inst if control instruction
       * @return error code
       */
      virtual int execute_if_ctrl(SpIfCtrlInsts *inst);
      /**
       * @brief execute_loop
       * execute loop instruction
       * @param inst loop instruction
       * @return error code
       */
      virtual int execute_loop(SpLoopInst *inst);
      /**
       * @brief execute_casewhen
       * execute case when instruction
       * @param inst case when instruction
       * @return error code
       */
      virtual int execute_casewhen(SpCaseInst *inst);
      /**
       * @brief execute_while
       * execute while instruction
       * @param inst while instruction
       * @return error code
       */
      virtual int execute_while(SpWhileInst *inst);
      /**
       * @brief execute_exit
       * execute exit instruction
       * @param inst exit instruction
       * @return error code
       */
      virtual int execute_exit(SpExitInst *inst);

      /**
       * @brief execute_multi_inst
       * execute multiple instructions
       * @param mul_inst multiple instructions
       * @return error code
       */
      virtual int execute_multi_inst(SpMultiInsts *mul_inst);
      /**
       * @brief init_physical_plan
       * initialize physical plan
       * @param exec_plan execution plan
       * @param out_plan have result set
       * @return error code
       */
      int init_physical_plan(ObPhysicalPlan &exec_plan, ObPhysicalPlan &out_plan);
      /**
       * @brief set_trans_params
       * set transcation params
       * @param session sql session
       * @param req transcation requestion
       * @return error code
       */
      int set_trans_params(ObSQLSessionInfo *session, common::ObTransReq &req);
      /**
       * @brief handle_group_result
       * handle the group execution result
       * @param proc point SpProcedure object
       * @param result ups execute result
       * @return error code
       */
      int handle_group_result(SpProcedure *proc, ObUpsResult &result);
      /**
       * @brief execute_pre_group
       * execute prepare group instruction
       * @param inst
       * @return error code
       */
      int execute_pre_group(SpPreGroupInsts *inst);

    private:
      //execution context
      ObLoopCounter loop_counter_;  ///<  loop counter
      ObRow curr_row_;   ///<  current row
      ObRowDesc fake_row_desc_;  ///<  row descriptor
      ObStringBuf obj_pool_;  ///<  object pool
    };

    /**
     * @brief The ObProcedure class
     * ObProcedure is the wrapper of a stored procedure, the really execution model is include
     * in this class, but the execution model could not be the iterator model
     *
     * the real execution plan is owned by the procedure, instead of sp inst
     *
     * ObProcedure is the sub-class of SpProcedure that executed on ms
     */
    class ObProcedure : public SpProcedure
		{
    public:
      friend class ObProcedureOptimizer;  ///< friend class
      /**
       * @brief constructor
       */
			ObProcedure();
      /**
       * @brief destructor
       */
			virtual ~ObProcedure();
      /**
       * @brief reset
       * clear object
       */
			virtual void reset();
      /**
       * @brief reuse
       * clear object
       */
			virtual void reuse();
      /**
       * @brief open
       * important function  begin to execute phyoperator
       * @return error code
       */
      virtual int open();
      /**
       * @brief close
       * close phyoperator
       * @return
       */
      virtual int close();
      /**
       * @brief to_string
       * @param buf buffer
       * @param buf_len buffer length
       * @return byte number
       */
      virtual int64_t to_string(char* buf, const int64_t buf_len) const;
      /**
       * @brief get_row_desc
       * get row descriptor
       * @param row_desc  row descriptor
       * @return error code
       */
			virtual int get_row_desc(const common::ObRowDesc *&row_desc) const;
      /**
       * @brief get_next_row
       * get next row
       * @param row
       * @return error code
       */
			virtual int get_next_row(const common::ObRow *&row);
      /**
       * @brief set_rpc_stub
       * set rpc proxy
       * @param rpc point ObMergerRpcProxy object
       * @return  error code
       */
      int set_rpc_stub(mergeserver::ObMergerRpcProxy *rpc) { rpc_ = rpc; return OB_SUCCESS;}
      /**
       * @brief get_rpc_stub
       * get rpc proxy
       * @return  ObMergerRpcProxy object point
       */
      mergeserver::ObMergerRpcProxy * get_rpc_stub() { return rpc_; }
      /**
       * @brief add_param
       * add a param define
       * @param proc_param ObParamDef object
       * @return error code
       */
      int add_param(const ObParamDef &proc_param);	//add param def
      /**
       * @brief add_var_def
       * add a variable define
       * @param def ObVariableDef object
       * @return error code
       */
      int add_var_def(const ObVariableDef &def);
      /**
       * @brief fill_parameters
       * fill parameters to phyoperator
       * @param param_expr param exprission array
       * @return error code
       */
      int fill_parameters(ObIArray<ObSqlExpression> &param_expr);
      /**
       * @brief return_paramters
       * return paramter into session
       * @return error code
       */
      int return_paramters();
      /**
       * @brief create_variables
       * create procedure variables
       * @return error code
       */
      int create_variables();
//      int clear_variables();

      /*
      virtual int write_variable(const ObString &var_name, const ObObj & val);
      virtual int write_variable(const ObString &array_name, int64_t idx_value, const ObObj &val);
      virtual int write_variable(const SpVar &var, const ObObj &val);

      virtual int read_variable(const ObString &var_name, const ObObj *&val) const;
      virtual int read_variable(const ObString &array_name, int64_t idx_value, const ObObj *&val) const;
      virtual int read_variable(const SpVar &var, const ObObj *&val) const;

      virtual int read_array_size(const ObString &array_name, int64_t &size) const;
      */

      /*
      virtual int store_static_data(int64_t sdata_id, int64_t hkey, ObRowStore *&p_row_store);
      virtual int get_static_data_by_id(int64_t sdata_id, ObRowStore *&p_row_store);
      virtual int get_static_data(int64_t idx, int64_t &sdata_id, int64_t &hkey, const ObRowStore *&p_row_store);
      virtual int64_t get_static_data_count() const;
      */
      /**
       * @brief hkey
       * The hash value of the iteration number of the corresponding cycle.
       * It is used to distinguish the same baseline operation,
       * and the baseline data generated at different times of the loop iteration.
       * @param sdata_id unique identifier corresponding to baseline operation
       * @return
       */
      virtual int64_t hkey(int64_t sdata_id) const;
      /**
       * @brief deter_exec_mode
       * This function determines the execution mode of the RPC in the stored procedure.
       * @return error code
       */
      int deter_exec_mode();
      /**
       * @brief get_param
       * get param by id
       * @param index param index
       * @return ObParamDef object
       */
      const ObParamDef& get_param(int64_t index) const;
      /**
       * @brief get_param_num
       * get params number
       * @return
       */
      int64_t get_param_num() const;
      /**
       * @brief assign
       * pyhoperator assgin operation
       * @param other other pyhoperator
       * @return error code
       */
      DECLARE_PHY_OPERATOR_ASSIGN;
      /**
       * @brief set_inst_op
       * set instruction op_
       * @return error code
       */
      int set_inst_op();
      /**
       * @brief end_trans
       * transcation end
       * @param rollback rollback flag
       * @return
       */
      int end_trans(bool rollback);
      /**
       * @brief check_semantics
       * @return
       */
      int check_semantics() const;
		private:
			//disallow copy
      /**
       * @brief copy constructor disable as private attribute
       * @param other ObProcedure object
       */
			ObProcedure(const ObProcedure &other);
      /**
       * @brief operator =
       * = operator overload
       * @param other
       * @return
       */
			ObProcedure& operator=(const ObProcedure &other);
      //function members
      /**
       * @brief ObProcedure::set_inst_op
       * If the procedure is constructed by assign methods,
       * each instrcution only have the query_id, the op_ filed would be null
       * we need to set the op_ according to the query_id
       * @param procedure instruction
       * @return error code
       */
      int set_inst_op(SpInst *inst);
      /**
       * @brief is_defined
       * check variable whether difined
       * @param info variable
       * @return bool value
       */
      bool is_defined(const SpVarInfo &info) const;
    private:
      ObSEArray<ObParamDef, 16> params_;  ///<  procedure params
      ObSEArray<ObVariableDef, 16> defs_;  ///<  procedure defined variables

      SpInstList exec_list_;
//      ObProcedureStaticDataMgr static_data_mgr_;


      SpMsInstExecStrategy *strategy_;  ///<  MS instrcution execute strategy


      bool long_trans_;  ///<  long transcation that more than one group commit
      mergeserver::ObMergerRpcProxy *rpc_;  ///< ms rpc proxy
    };
  }
}

#endif
