/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ClassHierarchy.h
 *
 *  Created on: 01.02.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_LLVMTYPEHIERARCHY_H_
#define ANALYSIS_LLVMTYPEHIERARCHY_H_

#include <algorithm>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <map>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>
#include <phasar/Utils/Logger.h>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace psr {

/**
 * 	@brief Owns the class hierarchy of the analyzed program.
 *
 * 	This class is responsible for constructing a inter-modular class
 * 	hierarchy graph based on the data from the %ProjectIRCompiledDB
 * 	and reconstructing the virtual method tables.
 */
class LLVMTypeHierarchy {
public:
  /// necessary for storing/loading the LLVMTypeHierarchy to/from database
  friend class DBConn;

  struct VertexProperties {
    llvm::Type *llvmtype = nullptr;
    /// always StructType so far - is it used anywhere???
    /// Name of the class/struct the vertex is representing.
    std::string name;
  };

  /// Edges in the class hierarchy graph doesn't hold any additional
  /// information.
  struct EdgeProperties {
    EdgeProperties() = default;
  };

  /// Data structure holding the class hierarchy graph.
  typedef boost::adjacency_list<boost::setS, boost::vecS, boost::bidirectionalS,
                                VertexProperties, EdgeProperties>
      bidigraph_t;

  /// The type for vertex representative objects.
  typedef boost::graph_traits<bidigraph_t>::vertex_descriptor vertex_t;
  /// The type for edge representative objects.
  typedef boost::graph_traits<bidigraph_t>::edge_descriptor edge_t;
  // Let us have some further handy typedefs.
  typedef boost::graph_traits<bidigraph_t>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<bidigraph_t>::out_edge_iterator out_edge_iterator;
  typedef boost::graph_traits<bidigraph_t>::in_edge_iterator in_edge_iterator;

private:
  struct reachability_dfs_visitor : boost::default_dfs_visitor {
    std::set<vertex_t> &subtypes;
    reachability_dfs_visitor(std::set<vertex_t> &types) : subtypes(types) {}
    template <typename Vertex, typename Graph>
    void finish_vertex(Vertex u, const Graph &g) {
      subtypes.insert(u);
    }
  };

  bidigraph_t g;
  std::map<std::string, vertex_t> type_vertex_map;
  // maps type names to the corresponding vtable
  std::map<std::string, VTable> vtable_map;
  std::set<std::string> recognized_struct_types;
  // holds all modules that are included in the type hierarchy
  std::set<const llvm::Module *> contained_modules;

  void reconstructVTable(const llvm::Module &M);
  FRIEND_TEST(VTableTest, SameTypeDifferentVTables);

public:
  /**
   * 	@brief Creates an empty LLVMStructTypeHierarchy.
   *
   * 	Is used, when re-storing type hierarchy from database.
   */
  LLVMTypeHierarchy() = default;

  /**
   *  @brief Creates a LLVMStructTypeHierarchy based on the
   *         given ProjectIRCompiledDB.
   *  @param IRDB ProjectIRCompiledDB object.
   */
  LLVMTypeHierarchy(ProjectIRDB &IRDB);

  ~LLVMTypeHierarchy() = default;

  /**
   * @brief Constructs the actual class hierarchy graph.
   * @param M LLVM module
   *
   * Extracts new information from the given module and adds new vertices
   * and edges accordingly to the type hierarchy graph.
   */
  void analyzeModule(const llvm::Module &M);

  /**
   * @brief Transform the type name to use it.
   * @param TypeName Name of the type.
   *
   * Erase the 'class.' or 'struct.' at the head of type names.
   */
  void inline uniformTypeName(std::string &TypeName) const;

  /**
   * 	@brief Computes all types, which are transitiv reachable from
   * 	       the given type.
   * 	@param TypeName Name of the type.
   * 	@return Set of reachable types.
   */
  std::set<std::string> getTransitivelyReachableTypes(std::string TypeName);
  // not used?
  std::vector<const llvm::Function *> constructVTable(const llvm::Type *T,
                                                      const llvm::Module *M);

  /**
   * 	@brief Returns an entry at the given index from the VTable
   * 	       of the given type.
   * 	@param TypeName Type identifier.
   * 	@param idx Index in the VTable.
   * 	@return A function identifier.
   */
  std::string getVTableEntry(std::string TypeName, unsigned idx);

  /**
   * 	@brief Checks if one of the given types is a super-type of the
   * 	       other given type.
   * 	@param TypeName Type identifier.
   * 	@param SubTypeName Type identifier.
   * 	@return True, if the one type is a super-type of the other.
   * 	        False otherwise.
   *
   * 	NOT YET SUPPORTED!
   */
  bool hasSuperType(std::string TypeName, std::string SuperTypeName);

  VTable getVTable(std::string TypeName);

  /**
   * 	@brief Checks if one of the given types is a sub-type of the
   * 	       other given type.
   * 	@param TypeName Type identifier.
   * 	@param SubTypeName Type identifier.
   * 	@return True, if the one type is a sub-type of the other.
   * 	        False otherwise.
   */
  bool hasSubType(std::string TypeName, std::string SubTypeName);

  /**
   *	@brief Checks if the given type has a virtual method table.
   *	@param TypeName Type identifier.
   *	@return True, if the given type has a virtual method table.
   *	        False otherwise.
   */
  bool containsVTable(std::string TypeName) const;

  /**
   * 	@brief Prints the transitive closure of the class hierarchy graph.
   */
  void printTransitiveClosure();

  /**
   * 	@brief Prints the class hierarchy to the command-line.
   */
  void print();
  /**
   * 	@brief Prints the class hierarchy to a .dot file.
   * 	@param path Path where the .dot file is created.
   */
  void printAsDot(const std::string &path = "struct_type_hierarchy.dot");

  bool containsType(std::string TypeName);

  std::string getPlainTypename(std::string TypeName);

  void printGraphAsDot(std::ostream &out);

  static bidigraph_t loadGraphFormDot(std::istream &in);

  json getAsJson();

  unsigned getNumOfVertices();

  unsigned getNumOfEdges();

  // these are defined in the DBConn class
  /**
   * 	@brief %LLVMStructTypeHierarchy store operator.
   * 	@param db SQLite3 database to store the class hierarchy in.
   * 	@param STH %LLVMStructTypeHierarchy object that is stored.
   *
   * 	By storing the class hierarchy in the database, a repeated
   * 	reconstruction of the class hierarchy graph as well as the
   * 	VTables from the corresponding LLVM module(s) is unnecessary.
   *
   * 	To store the class hierarchy graph itself, a %Hexastore data
   * 	structure is used.
   */
  // friend void operator<<(DBConn& db, const LLVMTypeHierarchy& STH);

  /**
   * 	@brief %LLVMStructTypeHierarchy load operator.
   * 	@param db SQLite3 database the class hierarchy is stored in.
   * 	@param STH %LLVMStructTypeHierarchy object that is restored.
   */
  // friend void operator>>(DBConn& db, LLVMTypeHierarchy& STH);
};

} // namespace psr

#endif /* ANALYSIS_LLVMTYPEHIERARCHY_HH_ */
