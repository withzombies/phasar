/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef DATAFLOWANALYSISTYPE_H_
#define DATAFLOWANALYSISTYPE_H_

#include <iostream>
#include <map>
#include <string>

namespace psr {

enum class DataFlowAnalysisType {
  IFDS_UninitializedVariables = 0,
  IFDS_ConstAnalysis,
  IFDS_TaintAnalysis,
  IDE_TaintAnalysis,
  IDE_TypeStateAnalysis,
  IFDS_TypeAnalysis,
  IFDS_SolverTest,
  IFDS_LinearConstantAnalysis,
  IDE_LinearConstantAnalysis,
  IDE_SolverTest,
  MONO_Intra_FullConstantPropagation,
  MONO_Intra_SolverTest,
  MONO_Inter_SolverTest,
  Plugin,
  None
};

extern const std::map<std::string, DataFlowAnalysisType>
    StringToDataFlowAnalysisType;

extern const std::map<DataFlowAnalysisType, std::string>
    DataFlowAnalysisTypeToString;

std::ostream &operator<<(std::ostream &os, const DataFlowAnalysisType &k);

} // namespace psr

#endif
