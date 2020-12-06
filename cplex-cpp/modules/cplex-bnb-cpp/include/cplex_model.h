#pragma once
// Magic tricks to have CPLEX behave well:
#ifndef IL_STD
#define IL_STD
#endif

#include <ilcplex/ilocplex.h>
#include <set>

#include <include/cql_graph.h>
#include "shared.h"


class CplexModel {
private:
    // CPLEX environment. Takes care of everything, including memory management for CPLEX objects.
    IloEnv env;

    // CPLEX model. We put variables and constraints in it!
    IloModel model;

    IloNumVarArray x;

    IloExpr expr;

    std::stringstream names_stream;

    IloCplex cplex;

    std::vector<IloRange> all_constraints;

    IloRange buildConstraint(const std::set<uint64_t> &constraint, double lower_bound, double upper_bound);

public:
    explicit CplexModel(std::size_t variables_num);

    void addConstraints(const std::set<std::set<uint64_t>> &constraints, double lower_bound, double upper_bound);

    void reduceModel(std::size_t limit = 1000);

    IloRange addEqualityConstraintToVariable(uint64_t variable, double equals_to);

    FloatSolution getFloatSolution();

    void deleteConstraint(const IloRange &constrain);

    void addConstraint(const IloRange &constraint);
};
