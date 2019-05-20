/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libsolidity/formal/Z3CHCInterface.h>

#include <liblangutil/Exceptions.h>
#include <libdevcore/CommonIO.h>

using namespace std;
using namespace dev;
using namespace dev::solidity::smt;

Z3CHCInterface::Z3CHCInterface():
	m_solver(m_context),
	m_variables(m_context)
{
	// This needs to be set globally.
	z3::set_param("rewriter.pull_cheap_ite", true);
	// This needs to be set in the context.
	m_context.set("timeout", queryTimeout);

	z3::params p(m_context);
	p.set("print_certificate", true);
	m_solver.set(p);
}

void Z3CHCInterface::reset()
{
	Z3Interface::reset();
}

void Z3CHCInterface::push()
{
	CHCSolverInterface::push();
}

void Z3CHCInterface::pop()
{
	CHCSolverInterface::pop();
}

void Z3CHCInterface::declareVariable(string const& _name, Sort const& _sort)
{
	Z3Interface::declareVariable(_name, _sort);
	if (_sort.kind != Kind::Function)
		m_variables.push_back(m_constants.at(_name));
}

void Z3CHCInterface::addAssertion(Expression const& _expr)
{
	CHCSolverInterface::addAssertion(_expr);
}

pair<CheckResult, vector<string>> Z3CHCInterface::check(vector<Expression> const& _expressionsToEvaluate)
{
	solAssert(_expressionsToEvaluate.size() == 1, "");
	return query(_expressionsToEvaluate.front());
}

void Z3CHCInterface::registerRelation(Expression const& _expr)
{
	m_solver.register_relation(m_functions.at(_expr.name));
}

void Z3CHCInterface::addRule(Expression const& _expr, string const& _name)
{
	z3::expr rule = toZ3Expr(_expr);
	z3::expr boundRule = z3::forall(m_variables, rule);
	m_solver.add_rule(boundRule, m_context.str_symbol(_name.c_str()));
}

pair<CheckResult, vector<string>> Z3CHCInterface::query(Expression const& _expr)
{
	cout << m_solver << endl;
	CheckResult result;
	vector<string> values;
	try
	{
		z3::expr z3Expr = toZ3Expr(_expr);
		switch (m_solver.query(z3Expr))
		{
		case z3::check_result::sat:
		{
			result = CheckResult::SATISFIABLE;
			//auto model = m_solver.get_answer();
			//cout << model << endl;
			break;
		}
		case z3::check_result::unsat:
			result = CheckResult::UNSATISFIABLE;
			break;
		case z3::check_result::unknown:
			result = CheckResult::UNKNOWN;
			break;
		}
		// TODO retrieve model / invariants
	}
	catch (z3::exception const& _e)
	{
		result = CheckResult::ERROR;
		values.clear();
	}

	return make_pair(result, values);
}
